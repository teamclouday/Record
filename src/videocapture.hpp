#pragma once
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include "streams.hpp"

#include <array>
#include <memory>
#include <string>

/** @file */

/// Video capture default FPS
#define VIDEO_DEFAULT_FPS 30

/// Video capture default bit rate
#define VIDEO_DEFAULT_BITRATE 4000000

/**
 * @brief Video Capture
 *
 * This class handles video capture, decode & encode.
 *
 */
class VideoCapture
{
  public:
    VideoCapture();
    ~VideoCapture();

    bool openCapture(AVFormatContext *oc, const std::array<int, 4> &window);
    bool closeCapture();
    bool writeFrame(AVFormatContext *oc, bool skip);

    void UI();

    const std::string NAME = "VideoCapture";

  private:
    bool openDevice();
    bool configIStream();
    bool configOStream(AVFormatContext *oc);
    bool decode(AVCodecContext *codecCtx, AVFrame *frame, AVPacket *pkt);
    bool encode(AVCodecContext *codecCtx, AVFrame *frame, AVPacket *pkt);

    std::unique_ptr<InputStream> _ist;
    std::unique_ptr<OutputStream> _ost;

    // x, y, w, h, fps, bitrate
    std::array<int, 6> _configs;
    bool _autoBitrate;
};
