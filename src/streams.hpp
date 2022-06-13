#pragma once
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

/** @file */

// reference: https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/muxing.c

/**
 * @brief Input Stream
 *
 * This structure stores input stream info.
 */
struct InputStream
{
    AVFormatContext *fmtCtx;
    AVCodecContext *decCtx;
    AVFrame *frame;
    AVPacket *pkt;
    int32_t streamIdx;

    InputStream() : fmtCtx(nullptr), decCtx(nullptr), frame(nullptr), pkt(nullptr), streamIdx(-1)
    {
    }

    ~InputStream()
    {
        if (fmtCtx)
            avformat_free_context(fmtCtx);
        if (decCtx)
            avcodec_free_context(&decCtx);
        if (frame)
            av_frame_free(&frame);
        if (pkt)
            av_packet_free(&pkt);
    }
};

/**
 * @brief Output Stream
 *
 * This structure stores output stream info.
 */
struct OutputStream
{
    AVCodecContext *encCtx;
    AVFrame *frame;
    AVPacket *pkt;
    AVStream *st;
    struct SwsContext *swsCtx;
    struct SwrContext *swrCtx;
    int64_t samples;

    OutputStream()
        : encCtx(nullptr), frame(nullptr), pkt(nullptr), st(nullptr), swsCtx(nullptr), swrCtx(nullptr), samples(0)
    {
    }

    ~OutputStream()
    {
        if (encCtx)
            avcodec_free_context(&encCtx);
        if (frame)
            av_frame_free(&frame);
        if (pkt)
            av_packet_free(&pkt);
        if (swsCtx)
            sws_freeContext(swsCtx);
        if (swrCtx)
            swr_free(&swrCtx);
    }
};

/**
 * @brief Filter Stream
 *
 * This structure stores audio filter info.
 */
struct FilterStream
{
    AVFilterGraph *graph;
    AVFilterContext *mixCtx;
    AVFilterContext *bufOutCtx;
    AVFilterContext *bufMicCtx;
    AVFilterContext *sinkCtx;
    AVFrame *frame;

    FilterStream()
        : graph(nullptr), mixCtx(nullptr), bufOutCtx(nullptr), bufMicCtx(nullptr), sinkCtx(nullptr), frame(nullptr)
    {
    }

    ~FilterStream()
    {
        if (graph)
            avfilter_graph_free(&graph);
        if (frame)
            av_frame_free(&frame);
    }
};
