#include "videocapture.hpp"
#include "utils.hpp"

// reference:
// https://github.com/leandromoreira/ffmpeg-libav-tutorial
// reference:
// https://github.com/abdullahfarwees/screen-recorder-ffmpeg-cpp/blob/master/src/ScreenRecorder.cpp
// reference:
// https://stackoverflow.com/questions/70390402/why-ffmpeg-screen-recorder-output-shows-green-screen-only

VideoCapture::VideoCapture() : _autoBitrate(true)
{
    _configs = {0, 0, 0, 0, VIDEO_DEFAULT_FPS, VIDEO_DEFAULT_BITRATE};
}

VideoCapture::~VideoCapture()
{
    closeCapture();
}

bool VideoCapture::openCapture(AVFormatContext *oc, const std::array<int, 4> &window)
{
    // update capture window configs
    for (int i = 0; i < 4; i++)
        _configs[i] = window[i];
    // refresh streams
    _ist = std::make_unique<InputStream>();
    _ost = std::make_unique<OutputStream>();
    bool success = true;
    // open capture device
    success = success && openDevice();
    // config input (decoder) context & stream
    success = success && configIStream();
    // config output (encoder) context & stream
    success = success && configOStream(oc);
    return success;
}

bool VideoCapture::closeCapture()
{
    if (_ist && _ist->fmtCtx)
        avformat_close_input(&_ist->fmtCtx);
    _ist = nullptr;
    _ost = nullptr;
    return true;
}

bool VideoCapture::writeFrame(AVFormatContext *oc, bool skip)
{
    if (av_read_frame(_ist->fmtCtx, _ist->pkt) < 0)
        return false;
    if (_ist->pkt->stream_index == _ist->streamIdx && !skip)
    {
        _ost->samples++;
        if (decode(_ist->decCtx, _ist->frame, _ist->pkt))
        {
            av_frame_make_writable(_ost->frame);
            sws_scale(_ost->swsCtx, _ist->frame->data, _ist->frame->linesize, 0, _ist->decCtx->height,
                      _ost->frame->data, _ost->frame->linesize);
            _ost->pkt->data = nullptr;
            _ost->pkt->size = 0;
            _ost->frame->pts = _ost->samples;
            if (encode(_ost->encCtx, _ost->frame, _ost->pkt))
            {
                av_packet_rescale_ts(_ost->pkt, _ost->encCtx->time_base, _ost->st->time_base);
                _ost->pkt->stream_index = _ost->st->index;
                if (av_interleaved_write_frame(oc, _ost->pkt) != 0)
                    display_message(NAME, "failed to write frame", MESSAGE_WARN);
                av_packet_unref(_ost->pkt);
            }
        }
    }
    return true;
}

bool VideoCapture::openDevice()
{
    std::string captureSource = "";
    std::string captureURL = "";
    AVDictionary *options{nullptr};
    {
        auto size = std::to_string(_configs[2]) + "x" + std::to_string(_configs[3]);
        av_dict_set(&options, "video_size", size.c_str(), 0);
        av_dict_set(&options, "framerate", std::to_string(_configs[4]).c_str(), 0);
    }
    // prepare capture source
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    // use gdi on windows
    captureSource = "gdigrab";
    captureURL = "desktop";
    {
        av_dict_set(&options, "offset_x", std::to_string(_configs[0]).c_str(), 0);
        av_dict_set(&options, "offset_y", std::to_string(_configs[1]).c_str(), 0);
    }
#elif __linux__
    // by default assume X11 backend (wayland won't work)
    captureSource = "x11grab";
    {
        av_dict_set(&options, "grab_x", std::to_string(_configs[0]).c_str(), 0);
        av_dict_set(&options, "grab_y", std::to_string(_configs[1]).c_str(), 0);
    }
#else
    display_message(NAME, "unsupported capture platform!", MESSAGE_WARN);
    return false;
#endif
    // get format and try to open
    auto formatIn = av_find_input_format(captureSource.c_str());
    if (0 != avformat_open_input(&_ist->fmtCtx, captureURL.c_str(), formatIn, &options))
    {
        display_message(NAME, "failed to open capture source " + captureSource, MESSAGE_WARN);
        return false;
    }
    return true;
}

bool VideoCapture::configIStream()
{
    // find video stream index
    {
        if (avformat_find_stream_info(_ist->fmtCtx, nullptr) < 0)
        {
            display_message(NAME, "failed to get stream info", MESSAGE_WARN);
            return false;
        }
        _ist->streamIdx = -1;
        for (unsigned i = 0; i < _ist->fmtCtx->nb_streams; i++)
        {
            if (_ist->fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                _ist->streamIdx = i;
                break;
            }
        }
        if (_ist->streamIdx < 0)
        {
            display_message(NAME, "failed to get stream index", MESSAGE_WARN);
            return false;
        }
    }
    // prepare codec
    auto param = _ist->fmtCtx->streams[_ist->streamIdx]->codecpar;
    auto codecIn = avcodec_find_decoder(param->codec_id);
    auto codecName = std::string(avcodec_get_name(param->codec_id));
    {
        if (!codecIn)
        {
            display_message(NAME, "failed to find decoder for " + codecName, MESSAGE_WARN);
            return false;
        }
        _ist->decCtx = avcodec_alloc_context3(codecIn);
        if (!_ist->decCtx)
        {
            display_message(NAME, "failed to allocate decoder for " + codecName, MESSAGE_WARN);
            return false;
        }
    }
    // open codec
    {
        if (avcodec_parameters_to_context(_ist->decCtx, param) < 0)
        {
            display_message(NAME, "failed to copy decoder params for " + codecName, MESSAGE_WARN);
            return false;
        }
        if (avcodec_open2(_ist->decCtx, codecIn, nullptr) < 0)
        {
            display_message(NAME, "failed to open decoder for " + codecName, MESSAGE_WARN);
            return false;
        }
    }
    // prepare packet
    {
        _ist->pkt = av_packet_alloc();
        if (!_ist->pkt)
        {
            display_message(NAME, "failed to allocate decoder packet", MESSAGE_WARN);
            return false;
        }
    }
    // prepare frame
    {
        _ist->frame = av_frame_alloc();
        if (!_ist->frame)
        {
            display_message(NAME, "failed to allocate decoder frame", MESSAGE_WARN);
            return false;
        }
        _ist->frame->width = _ist->decCtx->width;
        _ist->frame->height = _ist->decCtx->height;
        _ist->frame->format = _ist->decCtx->pix_fmt;
        if (av_frame_get_buffer(_ist->frame, 0) < 0)
        {
            display_message(NAME, "failed to allocate decoder frame buffer", MESSAGE_WARN);
            return false;
        }
    }
    return true;
}

bool VideoCapture::configOStream(AVFormatContext *oc)
{
    _ost->samples = 0;
    // allocate parameters
    AVCodecParameters *param = avcodec_parameters_alloc();
    {
        if (!param)
        {
            display_message(NAME, "failed to allocate codec params", MESSAGE_WARN);
            return false;
        }
        if (_autoBitrate)
            _configs[5] = _configs[2] * _configs[3] * _configs[4] * 8;
        param->width = _configs[2];
        param->height = _configs[3];
        param->bit_rate = _configs[5];
        param->codec_id = oc->video_codec_id;
        // this is a temp fix for webm format to work
        if (param->codec_id == AV_CODEC_ID_VP9)
            param->codec_id = AV_CODEC_ID_VP8;
        param->codec_type = AVMEDIA_TYPE_VIDEO;
    }
    // prepare codec
    auto codecOut = avcodec_find_encoder(param->codec_id);
    auto codecName = std::string(avcodec_get_name(param->codec_id));
    {
        if (!codecOut)
        {
            display_message(NAME, "failed to find encoder for " + codecName, MESSAGE_WARN);
            return false;
        }
        _ost->encCtx = avcodec_alloc_context3(codecOut);
        if (!_ost->encCtx)
        {
            display_message(NAME, "failed to allocate encoder for " + codecName, MESSAGE_WARN);
            return false;
        }
    }
    // open codec
    {
        if (avcodec_parameters_to_context(_ost->encCtx, param) < 0)
        {
            display_message(NAME, "failed to copy encoder params for " + codecName, MESSAGE_WARN);
            return false;
        }
        switch (param->codec_id)
        {
        case AV_CODEC_ID_GIF:
            _ost->encCtx->pix_fmt = AV_PIX_FMT_RGB8;
            break;
        case AV_CODEC_ID_APNG:
            _ost->encCtx->pix_fmt = AV_PIX_FMT_RGBA;
            break;
        default:
            _ost->encCtx->pix_fmt = AV_PIX_FMT_YUV420P;
            break;
        }
        _ost->encCtx->gop_size = 12;
        _ost->encCtx->time_base = {1, _configs[4]};
        _ost->encCtx->framerate = {_configs[4], 1};
        if (avcodec_open2(_ost->encCtx, codecOut, nullptr) < 0)
        {
            display_message(NAME, "failed to open encoder for " + codecName, MESSAGE_WARN);
            return false;
        }
        if (oc->oformat->flags & AVFMT_GLOBALHEADER)
            _ost->encCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    // prepare stream
    {
        _ost->st = avformat_new_stream(oc, codecOut);
        if (!_ost->st)
        {
            display_message(NAME, "failed to open encoder stream", MESSAGE_WARN);
            return false;
        }
        if (avcodec_parameters_copy(_ost->st->codecpar, param) < 0)
        {
            display_message(NAME, "failed to copy encoder stream params", MESSAGE_WARN);
            return false;
        }
    }
    // prepare packet
    {
        _ost->pkt = av_packet_alloc();
        if (!_ost->pkt)
        {
            display_message(NAME, "failed to allocate encoder packet", MESSAGE_WARN);
            return false;
        }
    }
    // prepare frame
    {
        _ost->frame = av_frame_alloc();
        if (!_ost->frame)
        {
            display_message(NAME, "failed to allocate encoder frame", MESSAGE_WARN);
            return false;
        }
        _ost->frame->width = _ost->encCtx->width;
        _ost->frame->height = _ost->encCtx->height;
        _ost->frame->format = _ost->encCtx->pix_fmt;
        if (av_frame_get_buffer(_ost->frame, 0) < 0)
        {
            display_message(NAME, "failed to allocate encoder frame buffer", MESSAGE_WARN);
            return false;
        }
    }
    // prepare sws ctx
    if (_ist->decCtx)
    {
        _ost->swsCtx =
            sws_getContext(_ist->decCtx->width, _ist->decCtx->height, _ist->decCtx->pix_fmt, _ost->encCtx->width,
                           _ost->encCtx->height, _ost->encCtx->pix_fmt, SWS_BICUBIC, nullptr, nullptr, nullptr);
        if (!_ost->swsCtx)
        {
            display_message(NAME, "failed to prepare sws context", MESSAGE_WARN);
            return false;
        }
    }
    else
    {
        display_message(NAME, "input stream not allocated", MESSAGE_WARN);
        return false;
    }
    avcodec_parameters_free(&param);
    return true;
}

bool VideoCapture::decode(AVCodecContext *codecCtx, AVFrame *frame, AVPacket *pkt)
{
    int ret;
    char buf[512];
    if ((ret = avcodec_send_packet(codecCtx, pkt)) < 0)
    {
        display_message(NAME, "decoder packet (" + std::string(av_make_error_string(buf, 512, ret)) + ")",
                        MESSAGE_WARN);
        return false;
    }
    if ((ret = avcodec_receive_frame(codecCtx, frame)) < 0)
    {
        display_message(NAME, "decoder frame (" + std::string(av_make_error_string(buf, 512, ret)) + ")", MESSAGE_WARN);
        return false;
    }
    return true;
}

bool VideoCapture::encode(AVCodecContext *codecCtx, AVFrame *frame, AVPacket *pkt)
{
    int ret;
    char buf[512];
    if ((ret = avcodec_send_frame(codecCtx, frame) < 0))
    {
        display_message(NAME, "encoder frame (" + std::string(av_make_error_string(buf, 512, ret)) + ")", MESSAGE_WARN);
        return false;
    }
    if ((ret = avcodec_receive_packet(codecCtx, pkt)) < 0)
    {
        display_message(NAME, "encoder packet (" + std::string(av_make_error_string(buf, 512, ret)) + ")",
                        MESSAGE_WARN);
        return false;
    }
    return true;
}
