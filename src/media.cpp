#include "media.hpp"
#include "utils.hpp"
#include <stdexcept>
#include <sstream>

// reference: https://github.com/leandromoreira/ffmpeg-libav-tutorial
// reference: https://github.com/abdullahfarwees/screen-recorder-ffmpeg-cpp/blob/master/src/ScreenRecorder.cpp
// reference: https://stackoverflow.com/questions/70390402/why-ffmpeg-screen-recorder-output-shows-green-screen-only

MediaHandler::MediaHandler()
{
    // default values
    _fps = VIDEO_DEFAULT_FPS;
    _bitrate = VIDEO_DEFAULT_BITRATE;
    _outFilePath = VIDEO_DEFAULT_OUTPUT;
    _recording = false;
    // prepare libav
    avdevice_register_all();
}

MediaHandler::~MediaHandler()
{
    StopRecord();
    freeLibAV();
}

void MediaHandler::freeLibAV()
{
    // free allocated variables
    if(_ifmtCtx) avformat_free_context(_ifmtCtx);
    if(_ofmtCtx) avformat_free_context(_ofmtCtx);
    _ifmtCtx = _ofmtCtx = nullptr;
    // following should be freed by avcodec_free_context
    // if(_icodecParams) avcodec_parameters_free(&_icodecParams);
    // if(_ocodecParams) avcodec_parameters_free(&_ocodecParams);
    if(_icodecCtx) avcodec_free_context(&_icodecCtx);
    if(_ocodecCtx) avcodec_free_context(&_ocodecCtx);
    if(_ipacket) av_packet_free(&_ipacket);
    if(_opacket) av_packet_free(&_opacket);
    if(_iAVFrame) av_frame_free(&_iAVFrame);
    if(_oAVFrame) av_frame_free(&_oAVFrame);
}

bool MediaHandler::openCapture()
{
    std::string captureSource = "";
    // prepare capture source
#if PLATFORM_WIN
    // use gdi on windows
    captureSource = "gdigrab";
#elif PLATFORM_LINUX
    // by default assume X11 backend (wayland won't work)
    captureSource = "x11grab";
#else
    display_message(NAME, "unsupported capture platform!", MESSAGE_WARN);
    return false;
#endif
    // get format and try to open
    auto formatIn = av_find_input_format(captureSource.c_str());
    if(0 != avformat_open_input(&_ifmtCtx, nullptr, formatIn, &_options))
    {
        display_message(NAME, "failed to open capture source " + captureSource, MESSAGE_WARN);
        return false;
    }
    return true;
}

void MediaHandler::SelectOutputPath()
{
#if PLATFORM_WIN
    // TODO: implement code for windows
#elif PLATFORM_LINUX
    
#else
    // unsupported platform
    _outFilePath = VIDEO_DEFAULT_OUTPUT;
    display_message(NAME, "unsupported capture platform!", MESSAGE_WARN);
#endif
}

bool MediaHandler::prepareParams()
{
    _ocodecParams->width = _rW;
    _ocodecParams->height = _rH;
    _ocodecParams->bit_rate = _bitrate; // The average bitrate of the encoded data (in bits per second)
    _ocodecParams->codec_id = AV_CODEC_ID_MPEG4;
    _ocodecParams->codec_type = AVMEDIA_TYPE_VIDEO;
    _ocodecParams->format = AV_PIX_FMT_YUV420P;
    _ocodecParams->sample_aspect_ratio.num = 4;
    _ocodecParams->sample_aspect_ratio.den = 3;
    if(_options) av_dict_free(&_options);
    auto size = std::to_string(_rW) + "x" + std::to_string(_rH);
    if(av_dict_set(&_options,"framerate",std::to_string(_fps).c_str(),0) < 0) goto failedconfig;
    if(av_dict_set(&_options,"grab_x",std::to_string(_rX).c_str(),0) < 0) goto failedconfig;
    if(av_dict_set(&_options,"grab_y",std::to_string(_rY).c_str(),0) < 0) goto failedconfig;
    if(av_dict_set(&_options,"video_size",size.c_str(),0) < 0) goto failedconfig;
    return true;
failedconfig:
    display_message(NAME, "failed to configure capture device", MESSAGE_WARN);
    return false;
}

void MediaHandler::ConfigWindow(int x, int y, int w, int h)
{
    _rX = x;
    _rY = y;
    _rW = w;
    _rH = h;
}

void MediaHandler::StartRecord()
{
    StopRecord();
    freeLibAV();
    _icodecParams = avcodec_parameters_alloc();
    _ocodecParams = avcodec_parameters_alloc();
    // prepare parameters
    if(!prepareParams()) return;
    // try to open capture device
    if(!openCapture()) return;
    // find stream info
    if(avformat_find_stream_info(_ifmtCtx, nullptr) < 0)
    {
        display_message(NAME, "failed to get capture stream info", MESSAGE_WARN);
        return;
    }
    _videoStreamIdx = -1;
    for(unsigned i = 0; i < _ifmtCtx->nb_streams; i++)
    {
        if(_ifmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            _videoStreamIdx = i;
            break;
        }
    }
    if(_videoStreamIdx < 0)
    {
        display_message(NAME, "failed to get capture stream info", MESSAGE_WARN);
        return;
    }
    // prepare input codec
    _icodecParams = _ifmtCtx->streams[_videoStreamIdx]->codecpar;
    auto codecIn = avcodec_find_decoder(_icodecParams->codec_id);
    if(!codecIn)
    {
        display_message(NAME, "failed to prepare capture codec", MESSAGE_WARN);
        return;
    }
    _icodecCtx = avcodec_alloc_context3(codecIn);
    if(avcodec_parameters_to_context(_icodecCtx, _icodecParams) < 0)
    {
        display_message(NAME, "failed to prepare capture codec", MESSAGE_WARN);
        return;
    }
    if(avcodec_open2(_icodecCtx, codecIn, nullptr) < 0)
    {
        display_message(NAME, "failed to prepare capture codec", MESSAGE_WARN);
        return;
    }
    // get output format
    auto formatOut = av_guess_format(nullptr, _outFilePath.c_str(), nullptr);
    if(!formatOut)
    {
        display_message(NAME, "failed to get format for " + _outFilePath, MESSAGE_WARN);
        return;
    }
    if(avformat_alloc_output_context2(&_ofmtCtx, formatOut, nullptr, nullptr) < 0)
    {
        display_message(NAME, "failed to get format for " + _outFilePath, MESSAGE_WARN);
        return;
    }
    // prepare output codec
    auto codecOut = avcodec_find_encoder(_ocodecParams->codec_id);
    if(!codecOut)
    {
        display_message(NAME, "failed to prepare output codec", MESSAGE_WARN);
        return;
    }
    _ocodecCtx = avcodec_alloc_context3(codecOut);
    if(!_ocodecCtx)
    {
        display_message(NAME, "failed to prepare output codec", MESSAGE_WARN);
        return;
    }
    // create stream
    _videoStream = avformat_new_stream(_ofmtCtx, codecOut);
    if(!_videoStream)
    {
        display_message(NAME, "failed to prepare video stream", MESSAGE_WARN);
        return;
    }
    if(avcodec_parameters_copy(_videoStream->codecpar, _ocodecParams) < 0)
    {
        display_message(NAME, "failed to prepare video stream", MESSAGE_WARN);
        return;
    }
    // prepare output context
    if(avcodec_parameters_to_context(_ocodecCtx, _ocodecParams) < 0)
    {
        display_message(NAME, "failed to prepare output context", MESSAGE_WARN);
        return;
    }
    _ocodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    _ocodecCtx->gop_size = 3; // the number of pictures in a group of pictures
    _ocodecCtx->max_b_frames = 2;
    _ocodecCtx->time_base.num = 1;
    _ocodecCtx->time_base.den = _fps;
    if(avcodec_open2(_ocodecCtx, codecOut, nullptr) < 0)
    {
        display_message(NAME, "failed to prepare output context", MESSAGE_WARN);
        return;
    }
    if(_icodecCtx->codec_id == AV_CODEC_ID_H264)
	{
        av_opt_set(_ocodecCtx->priv_data, "preset", "slow", 0);
	}
    // open output file
    if(avio_open(&_ofmtCtx->pb, _outFilePath.c_str(), AVIO_FLAG_READ_WRITE) < 0)
    {
        display_message(NAME, "failed to prepare " + _outFilePath, MESSAGE_WARN);
        return;
    }
    if(_ofmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
    {
        _ocodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    if(avformat_write_header(_ofmtCtx, nullptr) < 0)
    {
        display_message(NAME, "failed to prepare " + _outFilePath, MESSAGE_WARN);
        return;
    }
    // prepare packet and frame
    _ipacket = av_packet_alloc();
    _opacket = av_packet_alloc();
    if(!_ipacket || !_opacket)
    {
        display_message(NAME, "failed to prepare media packet", MESSAGE_WARN);
        return;
    }
    _iAVFrame = av_frame_alloc();
    _oAVFrame = av_frame_alloc();
    if(!_iAVFrame || !_oAVFrame)
    {
        display_message(NAME, "failed to prepare frame data", MESSAGE_WARN);
        return;
    }
    _iAVFrame->width = _icodecCtx->width;
    _iAVFrame->height = _icodecCtx->height;
    _iAVFrame->format = _icodecParams->format;
    _oAVFrame->width = _ocodecCtx->width;
    _oAVFrame->height = _ocodecCtx->height;
    _oAVFrame->format = _ocodecParams->format;
    av_frame_get_buffer(_iAVFrame, 0);
    av_frame_get_buffer(_oAVFrame, 0);
    // prepare sws
    // _swsCtx = sws_alloc_context();
    // if(sws_init_context(_swsCtx, nullptr, nullptr) < 0)
    // {
    //     display_message(NAME, "failed to prepare frame sws", MESSAGE_WARN);
    //     return;
    // }
    // _swsCtx = sws_getContext(_icodecCtx->width, _icodecCtx->height, _icodecCtx->pix_fmt,
    //     _ocodecCtx->width, _ocodecCtx->height, _ocodecCtx->pix_fmt,
    //     SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
    // if(!_swsCtx)
    // {
    //     display_message(NAME, "failed to prepare frame sws", MESSAGE_WARN);
    //     return;
    // }
    // start thread
    _recordLoop = true;
    _recordT = std::thread([this]{recordInternal();});
}

void MediaHandler::StopRecord()
{
    _recordLoop = false;
    if(_recordT.joinable()) _recordT.join();
    if(_ifmtCtx) avformat_close_input(&_ifmtCtx);
}

void MediaHandler::recordInternal()
{
    _recording = true;
    display_message(NAME, "started recording", MESSAGE_INFO);
    // start reading frames
    while(_recordLoop && av_read_frame(_ifmtCtx, _ipacket) >= 0)
    {
        if(_ipacket->stream_index == _videoStreamIdx)
        {
            if(!decodeVideo(_icodecCtx, _iAVFrame, _ipacket))
            {
                // TODO: decide break or continue
                continue;
            }
            av_packet_unref(_opacket);
            _opacket->data = nullptr;
            _opacket->size = 0;
            if(!encodeVideo(_ocodecCtx, _oAVFrame, _opacket))
            {
                // TODO: decide break or continue
                continue;
            }
            if(_opacket->pts != AV_NOPTS_VALUE)
                _opacket->pts = av_rescale_q(_opacket->pts, _ocodecCtx->time_base, _videoStream->time_base);
            if(_opacket->dts != AV_NOPTS_VALUE)
                _opacket->dts = av_rescale_q(_opacket->dts, _ocodecCtx->time_base, _videoStream->time_base);
            if(av_write_frame(_ofmtCtx, _opacket) < 0)
                display_message(NAME, "failed to write frame", MESSAGE_WARN);
            av_packet_unref(_opacket);
        }
    }
    // stop and write end to file
    if(av_write_trailer(_ofmtCtx) != 0)
        display_message(NAME, "failed to write trailer to " + _outFilePath, MESSAGE_WARN);
    display_message(NAME, "stopped recording", MESSAGE_INFO);
    _recording = false;
}

bool MediaHandler::decodeVideo(AVCodecContext* icodecCtx, AVFrame* iframe, AVPacket* ipacket)
{
    int ret;
    char buf[512];
    if((ret = avcodec_send_packet(icodecCtx, ipacket)) < 0)
    {
        display_message(NAME, "decode video (" +
            std::string(av_make_error_string(buf, 512, ret)) + ")",
            MESSAGE_WARN);
        return false;
    }
    while(ret >= 0)
    {
        ret = avcodec_receive_frame(icodecCtx, iframe);
    }
    return true;
}

bool MediaHandler::encodeVideo(AVCodecContext* ocodecCtx, AVFrame* oframe, AVPacket* opacket)
{
    int ret;
    char buf[512];
    do
    {
        ret = avcodec_send_frame(ocodecCtx, oframe);
    } while (ret >= 0);
    if((ret = avcodec_receive_packet(ocodecCtx, opacket)) < 0)
    {
        display_message(NAME, "encode video (" +
            std::string(av_make_error_string(buf, 512, ret)) + ")",
            MESSAGE_WARN);
        return false;
    }
    return true;
}

bool MediaHandler::IsRecording()
{
    return _recording;
}