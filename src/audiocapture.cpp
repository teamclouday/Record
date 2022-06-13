#define __STDC_CONSTANT_MACROS
extern "C"
{
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
}

#include "audiocapture.hpp"
#include "utils.hpp"

#include <cstdio>
#include <cstring>

// reference: https://gist.github.com/MrArtichaut/11136813
// reference: https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/transcoding.c

AudioCapture::AudioCapture()
    : _captureOut(false), _captureMic(false), _autoBitRate(true), _sampleRate(AUDIO_DEFAULT_SAMPLE_RATE),
      _bitRate(AUDIO_DEFAULT_BITRATE)
{
#if __linux__
    _pulse = std::make_unique<PulseAudioHelper>();
#endif
}

AudioCapture::~AudioCapture()
{
    closeCapture();
}

bool AudioCapture::openCapture(AVFormatContext *oc)
{
    if (!_captureMic && !_captureOut)
        return true;
    // refresh streams
    _istOut = std::make_unique<InputStream>();
    _istMic = std::make_unique<InputStream>();
    _ost = std::make_unique<OutputStream>();
    _filter = std::make_unique<FilterStream>();
    bool success = true;
    if (_captureOut)
    {
        // open capture device
        success = success && openDevice(false);
        // config input (decoder) context & stream
        success = success && configIStream(_istOut.get());
    }
    if (_captureMic)
    {
        // open capture device
        success = success && openDevice(true);
        // config input (decoder) context & stream
        success = success && configIStream(_istMic.get());
    }
    if (_captureMic && _captureOut)
        success = success && configFilter();
    // config output (encoder) context & stream
    success = success && configOStream(oc);
    return success;
}

bool AudioCapture::closeCapture()
{
    if (_istOut && _istOut->fmtCtx)
        avformat_close_input(&_istOut->fmtCtx);
    if (_istMic && _istMic->fmtCtx)
        avformat_close_input(&_istOut->fmtCtx);
    _istOut = nullptr;
    _istMic = nullptr;
    _ost = nullptr;
    _filter = nullptr;
    return true;
}

bool AudioCapture::writeFrame(AVFormatContext *oc, bool skip, bool flush)
{
    if (!_captureMic && !_captureOut)
        return false;
    if (_captureOut && av_read_frame(_istOut->fmtCtx, _istOut->pkt) < 0)
        return false;
    if (_captureMic && av_read_frame(_istMic->fmtCtx, _istMic->pkt) < 0)
        return false;
    if (skip)
        return true;
    int nb_samples = 0;
    if (_captureOut && _captureMic)
    {
        if (flush)
        {
            av_packet_unref(_istOut->pkt);
            av_packet_unref(_istMic->pkt);
            _istOut->pkt->data = nullptr;
            _istMic->pkt->data = nullptr;
        }
        bool loop = true, packetSent1 = false, packetSent2 = false;
        while (loop)
        {
            loop = false;
            if (decode(_istOut->decCtx, _istOut->frame, _istOut->pkt, packetSent1) &&
                (av_buffersrc_add_frame(_filter->bufOutCtx, _istOut->frame) >= 0))
                loop = true;
            if (decode(_istMic->decCtx, _istMic->frame, _istMic->pkt, packetSent2) &&
                (av_buffersrc_add_frame(_filter->bufMicCtx, _istMic->frame) >= 0))
                loop = true;
            av_frame_make_writable(_ost->frame);
            while (loop && (av_buffersink_get_frame(_filter->sinkCtx, _filter->frame) >= 0))
            {
                if ((nb_samples = swr_convert(_ost->swrCtx, _ost->frame->data, _ost->frame->nb_samples,
                                              const_cast<const uint8_t **>(_filter->frame->data),
                                              _filter->frame->nb_samples) > 0))
                {
                    _ost->samples += nb_samples;
                    writePacket(oc);
                    while (swr_get_delay(_ost->swrCtx, _ost->encCtx->sample_rate) > _ost->frame->nb_samples)
                    {
                        if ((nb_samples = swr_convert(_ost->swrCtx, _ost->frame->data, _ost->frame->nb_samples, nullptr,
                                                      0)) <= 0)
                            break;
                        _ost->samples += nb_samples;
                        writePacket(oc);
                    }
                }
                av_frame_unref(_filter->frame);
            }
        }
    }
    else if (_captureOut)
    {
        if (flush)
        {
            av_packet_unref(_istOut->pkt);
            _istOut->pkt->data = nullptr;
        }
        bool packetSent = false;
        while (decode(_istOut->decCtx, _istOut->frame, _istOut->pkt, packetSent))
        {
            av_frame_make_writable(_ost->frame);
            if ((nb_samples =
                     swr_convert(_ost->swrCtx, _ost->frame->data, _ost->frame->nb_samples,
                                 const_cast<const uint8_t **>(_istOut->frame->data), _istOut->frame->nb_samples) > 0))
            {
                _ost->samples += nb_samples;
                writePacket(oc);
                while (swr_get_delay(_ost->swrCtx, _ost->encCtx->sample_rate) > _ost->frame->nb_samples)
                {
                    if ((nb_samples =
                             swr_convert(_ost->swrCtx, _ost->frame->data, _ost->frame->nb_samples, nullptr, 0)) <= 0)
                        break;
                    _ost->samples += nb_samples;
                    writePacket(oc);
                }
            }
        }
        av_packet_unref(_istOut->pkt);
    }
    else
    {
        if (flush)
        {
            av_packet_unref(_istMic->pkt);
            _istMic->pkt->data = nullptr;
        }
        bool packetSent = false;
        while (decode(_istMic->decCtx, _istMic->frame, _istMic->pkt, packetSent))
        {
            av_frame_make_writable(_ost->frame);
            if ((nb_samples =
                     swr_convert(_ost->swrCtx, _ost->frame->data, _ost->frame->nb_samples,
                                 const_cast<const uint8_t **>(_istMic->frame->data), _istMic->frame->nb_samples) > 0))
            {
                _ost->samples += nb_samples;
                writePacket(oc);
                while (swr_get_delay(_ost->swrCtx, _ost->encCtx->sample_rate) > _ost->frame->nb_samples)
                {
                    if ((nb_samples =
                             swr_convert(_ost->swrCtx, _ost->frame->data, _ost->frame->nb_samples, nullptr, 0)) <= 0)
                        break;
                    _ost->samples += nb_samples;
                    writePacket(oc);
                }
            }
        }
        av_packet_unref(_istMic->pkt);
    }
    return true;
}

const OutputStream *AudioCapture::getStream()
{
    return _ost.get();
}

bool AudioCapture::openDevice(bool isMic)
{
    std::string captureSource = "";
    std::string captureURL = "";
    // prepare capture source
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    // use dshow on windows
    // TODO: fill for windows
#elif __linux__
    // by default use pulse audio
    captureSource = "pulse";
    if (isMic)
        captureURL = std::to_string(_pulse->micIdx);
    else
        captureURL = std::to_string(_pulse->outIdx);
#else
    display_message(NAME, "unsupported capture platform!", MESSAGE_WARN);
    return false;
#endif
    auto formatIn = av_find_input_format(captureSource.c_str());
    auto ist = isMic ? _istMic.get() : _istOut.get();
    if (0 != avformat_open_input(&ist->fmtCtx, captureURL.c_str(), formatIn, nullptr))
    {
        display_message(NAME, "failed to open capture source " + captureSource, MESSAGE_WARN);
        return false;
    }
    return true;
}

bool AudioCapture::configIStream(InputStream *ist)
{
    // find video stream index
    {
        if (avformat_find_stream_info(ist->fmtCtx, nullptr) < 0)
        {
            display_message(NAME, "failed to get stream info", MESSAGE_WARN);
            return false;
        }
        ist->streamIdx = -1;
        for (unsigned i = 0; i < ist->fmtCtx->nb_streams; i++)
        {
            if (ist->fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
            {
                ist->streamIdx = i;
                break;
            }
        }
        if (ist->streamIdx < 0)
        {
            display_message(NAME, "failed to get stream index", MESSAGE_WARN);
            return false;
        }
    }
    // prepare codec
    auto param = ist->fmtCtx->streams[ist->streamIdx]->codecpar;
    auto codecIn = avcodec_find_decoder(param->codec_id);
    auto codecName = std::string(avcodec_get_name(param->codec_id));
    {
        if (!codecIn)
        {
            display_message(NAME, "failed to find decoder for " + codecName, MESSAGE_WARN);
            return false;
        }
        ist->decCtx = avcodec_alloc_context3(codecIn);
        if (!ist->decCtx)
        {
            display_message(NAME, "failed to allocate decoder for " + codecName, MESSAGE_WARN);
            return false;
        }
    }
    // open codec
    {
        if (avcodec_parameters_to_context(ist->decCtx, param) < 0)
        {
            display_message(NAME, "failed to copy decoder params for " + codecName, MESSAGE_WARN);
            return false;
        }
        if (avcodec_open2(ist->decCtx, codecIn, nullptr) < 0)
        {
            display_message(NAME, "failed to open decoder for " + codecName, MESSAGE_WARN);
            return false;
        }
        ist->decCtx->channel_layout = av_get_default_channel_layout(ist->decCtx->channels);
    }
    // prepare packet
    {
        ist->pkt = av_packet_alloc();
        if (!ist->pkt)
        {
            display_message(NAME, "failed to allocate decoder packet", MESSAGE_WARN);
            return false;
        }
    }
    // prepare frame
    {
        ist->frame = av_frame_alloc();
        if (!ist->frame)
        {
            display_message(NAME, "failed to allocate decoder frame", MESSAGE_WARN);
            return false;
        }
        ist->frame->channels = ist->decCtx->channels;
        ist->frame->sample_rate = ist->decCtx->sample_rate;
        ist->frame->format = ist->decCtx->sample_fmt;
        ist->frame->channel_layout = ist->decCtx->channel_layout;
        ist->frame->nb_samples = 10000;
        if (av_frame_get_buffer(ist->frame, 0) < 0)
        {
            display_message(NAME, "failed to allocate decoder frame buffer", MESSAGE_WARN);
            return false;
        }
    }
    return true;
}

bool AudioCapture::configFilter()
{
    // create graph
    {
        _filter->graph = avfilter_graph_alloc();
        if (!_filter->graph)
        {
            display_message(NAME, "failed to allocate mix filter graph", MESSAGE_WARN);
            return false;
        }
    }
    // input buffers
    char args[512];
    {
        auto filterBufOut = avfilter_get_by_name("abuffer");
        auto filterBufMic = avfilter_get_by_name("abuffer");
        if (!filterBufOut || !filterBufMic)
        {
            display_message(NAME, "failed to get abuffer filter", MESSAGE_WARN);
            return false;
        }
        // set args
        std::snprintf(args, sizeof(args), "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
                      _istOut->decCtx->time_base.num, _istOut->decCtx->time_base.den, _istOut->decCtx->sample_rate,
                      av_get_sample_fmt_name(_istOut->decCtx->sample_fmt), _istOut->decCtx->channel_layout);
        if (avfilter_graph_create_filter(&_filter->bufOutCtx, filterBufOut, "player", args, nullptr, _filter->graph) <
            0)
        {
            display_message(NAME, "failed to create abuffer filter for player", MESSAGE_WARN);
            return false;
        }
        // set args
        std::snprintf(args, sizeof(args), "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
                      _istMic->decCtx->time_base.num, _istMic->decCtx->time_base.den, _istMic->decCtx->sample_rate,
                      av_get_sample_fmt_name(_istMic->decCtx->sample_fmt), _istMic->decCtx->channel_layout);
        if (avfilter_graph_create_filter(&_filter->bufMicCtx, filterBufMic, "mic", args, nullptr, _filter->graph) < 0)
        {
            display_message(NAME, "failed to create abuffer filter for mic", MESSAGE_WARN);
            return false;
        }
    }
    // mix filter
    {
        auto filterMix = avfilter_get_by_name("amix");
        if (!filterMix)
        {
            display_message(NAME, "failed to get amix filter", MESSAGE_WARN);
            return false;
        }
        if (avfilter_graph_create_filter(&_filter->mixCtx, filterMix, "amix", nullptr, nullptr, _filter->graph) < 0)
        {
            display_message(NAME, "failed to create amix filter", MESSAGE_WARN);
            return false;
        }
    }
    // buffer sink
    {
        auto filterSink = avfilter_get_by_name("abuffersink");
        if (!filterSink)
        {
            display_message(NAME, "failed to get sink filter", MESSAGE_WARN);
            return false;
        }
        if (avfilter_graph_create_filter(&_filter->sinkCtx, filterSink, "sink", nullptr, nullptr, _filter->graph) < 0)
        {
            display_message(NAME, "failed to create sink filter", MESSAGE_WARN);
            return false;
        }
        const enum AVSampleFormat out_fmts[] = {AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE};
        const int64_t out_layouts[] = {AV_CH_LAYOUT_STEREO, -1};
        const int out_srs[] = {_sampleRate, -1};
        av_opt_set_int_list(_filter->sinkCtx, "sample_fmts", out_fmts, -1, AV_OPT_SEARCH_CHILDREN);
        av_opt_set_int_list(_filter->sinkCtx, "channel_layouts", out_layouts, -1, AV_OPT_SEARCH_CHILDREN);
        av_opt_set_int_list(_filter->sinkCtx, "sample_rates", out_srs, -1, AV_OPT_SEARCH_CHILDREN);
        if (avfilter_init_str(_filter->sinkCtx, nullptr) < 0)
        {
            display_message(NAME, "failed to init sink filter", MESSAGE_WARN);
            return false;
        }
    }
    // connect and configure graph
    {
        if ((avfilter_link(_filter->bufOutCtx, 0, _filter->mixCtx, 0) < 0) ||
            (avfilter_link(_filter->bufMicCtx, 0, _filter->mixCtx, 1) < 0) ||
            (avfilter_link(_filter->mixCtx, 0, _filter->sinkCtx, 0) < 0))
        {
            display_message(NAME, "failed to init filters", MESSAGE_WARN);
            return false;
        }
        if (avfilter_graph_config(_filter->graph, nullptr) < 0)
        {
            display_message(NAME, "failed to configure filter graph", MESSAGE_WARN);
            return false;
        }
    }
    // output frame
    {
        _filter->frame = av_frame_alloc();
        if (!_filter->frame)
        {
            display_message(NAME, "failed to allocate filter output frame", MESSAGE_WARN);
            return false;
        }
    }
    // dump graph
    {
        auto info = avfilter_graph_dump(_filter->graph, nullptr);
        if (info)
            display_message(NAME, "amix filter graph:\n" + std::string(info), MESSAGE_INFO);
    }
    return true;
}

bool AudioCapture::configOStream(AVFormatContext *oc)
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
        if (_autoBitRate)
            _bitRate = _sampleRate * 16 * AUDIO_OUTPUT_CHANNELS / 10;
        if (oc->audio_codec_id != AV_CODEC_ID_MP2)
            param->bit_rate = _bitRate;
        param->sample_rate = _sampleRate;
        param->channels = AUDIO_OUTPUT_CHANNELS;
        param->channel_layout = av_get_default_channel_layout(AUDIO_OUTPUT_CHANNELS);
        param->codec_id = oc->audio_codec_id;
        param->codec_type = AVMEDIA_TYPE_AUDIO;
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
        switch (_ost->encCtx->codec_id)
        {
        case AV_CODEC_ID_MP2:
        case AV_CODEC_ID_OPUS:
            _ost->encCtx->sample_fmt = AV_SAMPLE_FMT_S16;
            break;
        default:
            _ost->encCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
            break;
        }
        if (avcodec_open2(_ost->encCtx, codecOut, nullptr) < 0)
        {
            display_message(NAME, "failed to open encoder for " + codecName, MESSAGE_WARN);
            return false;
        }
        if (oc->oformat->flags & AVFMT_GLOBALHEADER)
            _ost->encCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        if (codecOut->supported_samplerates)
        {
            auto &sr = _ost->encCtx->sample_rate;
            sr = codecOut->supported_samplerates[0];
            for (int i = 1; codecOut->supported_samplerates[i]; i++)
            {
                auto currSR = codecOut->supported_samplerates[i];
                if ((std::abs)(currSR - _sampleRate) < (std::abs)(sr - _sampleRate))
                    sr = currSR;
            }
            if (sr != _sampleRate)
            {
                _sampleRate = sr;
                display_message(NAME, "sample rate reset to " + std::to_string(_sampleRate), MESSAGE_INFO);
            }
        }
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
        _ost->st->id = oc->nb_streams - 1;
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
        _ost->frame->channels = _ost->encCtx->channels;
        _ost->frame->channel_layout = _ost->encCtx->channel_layout;
        _ost->frame->sample_rate = _ost->encCtx->sample_rate;
        _ost->frame->format = _ost->encCtx->sample_fmt;
        if ((_ost->encCtx->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE))
            _ost->frame->nb_samples = 10000;
        else
            _ost->frame->nb_samples = _ost->encCtx->frame_size;
        if (av_frame_get_buffer(_ost->frame, 0) < 0)
        {
            display_message(NAME, "failed to allocate encoder frame buffer", MESSAGE_WARN);
            return false;
        }
    }
    // allocate resample context
    {
        _ost->swrCtx = swr_alloc();
        if (!_ost->swrCtx)
        {
            display_message(NAME, "failed to allocate resampler context", MESSAGE_WARN);
            return false;
        }
        if (_captureMic && _captureOut)
        {
            av_opt_set_int(_ost->swrCtx, "in_sample_rate", av_buffersink_get_sample_rate(_filter->sinkCtx), 0);
            av_opt_set_channel_layout(_ost->swrCtx, "in_channel_layout",
                                      av_buffersink_get_channel_layout(_filter->sinkCtx), 0);
            av_opt_set_sample_fmt(_ost->swrCtx, "in_sample_fmt",
                                  (AVSampleFormat)av_buffersink_get_format(_filter->sinkCtx), 0);
        }
        else
        {
            auto ist = _captureOut ? _istOut.get() : _istMic.get();
            av_opt_set_int(_ost->swrCtx, "in_sample_rate", ist->decCtx->sample_rate, 0);
            av_opt_set_channel_layout(_ost->swrCtx, "in_channel_layout", ist->decCtx->channel_layout, 0);
            av_opt_set_sample_fmt(_ost->swrCtx, "in_sample_fmt", ist->decCtx->sample_fmt, 0);
        }
        av_opt_set_int(_ost->swrCtx, "out_sample_rate", _ost->encCtx->sample_rate, 0);
        av_opt_set_channel_layout(_ost->swrCtx, "out_channel_layout", _ost->encCtx->channel_layout, 0);
        av_opt_set_sample_fmt(_ost->swrCtx, "out_sample_fmt", _ost->encCtx->sample_fmt, 0);
        if (swr_init(_ost->swrCtx) < 0)
        {
            display_message(NAME, "failed to init resampler context", MESSAGE_WARN);
            return false;
        }
    }
    avcodec_parameters_free(&param);
    return true;
}

bool AudioCapture::decode(AVCodecContext *codecCtx, AVFrame *frame, AVPacket *pkt, bool &packetSent)
{
    int ret;
    char buf[512];
    if (!packetSent && (ret = avcodec_send_packet(codecCtx, pkt)) < 0)
    {
        display_message(NAME, "decoder packet (" + std::string(av_make_error_string(buf, sizeof(buf), ret)) + ")",
                        MESSAGE_WARN);
        return false;
    }
    packetSent = true;
    return avcodec_receive_frame(codecCtx, frame) >= 0;
}

bool AudioCapture::encode(AVCodecContext *codecCtx, AVFrame *frame, AVPacket *pkt, bool &frameSent)
{
    int ret;
    char buf[512];
    if (!frameSent && (ret = avcodec_send_frame(codecCtx, frame) < 0))
    {
        display_message(NAME, "encoder frame (" + std::string(av_make_error_string(buf, sizeof(buf), ret)) + ")",
                        MESSAGE_WARN);
        return false;
    }
    frameSent = true;
    return avcodec_receive_packet(codecCtx, pkt) >= 0;
}

void AudioCapture::writePacket(AVFormatContext *oc)
{
    bool frameSent = false;
    _ost->frame->pts = av_rescale_q(_ost->samples, {1, _ost->encCtx->sample_rate}, _ost->encCtx->time_base);
    while (encode(_ost->encCtx, _ost->frame, _ost->pkt, frameSent))
    {
        av_packet_rescale_ts(_ost->pkt, _ost->encCtx->time_base, _ost->st->time_base);
        _ost->pkt->stream_index = _ost->st->index;
        if (av_interleaved_write_frame(oc, _ost->pkt) != 0)
            display_message(NAME, "failed to write frame", MESSAGE_WARN);
        av_packet_unref(_ost->pkt);
    }
}
