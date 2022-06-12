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
 */
class VideoCapture
{
  public:
    VideoCapture();
    ~VideoCapture();

    /**
     * @brief Open Video Capture
     *
     * Meant to be called from MediaHandler.
     *
     * @param oc Output format context
     * @param window Capture window configs
     * @return true if starts capture
     * @return false otherwise
     */
    bool openCapture(AVFormatContext *oc, const std::array<int, 4> &window);

    /**
     * @brief Close Video Capture
     *
     * Meant to be called from MediaHandler.
     *
     * @return true if success
     * @return false otherwise
     */
    bool closeCapture();

    /**
     * @brief Write Frame to Output
     *
     * Meant to be called from MediaHandler.
     *
     * @param oc Output format context
     * @param skip Whether to skip writing current frame
     * @return true if success
     * @return false otherwise
     */
    bool writeFrame(AVFormatContext *oc, bool skip);

    /**
     * @brief UI Calls
     *
     * Is meant to be called from MediaHandler.
     */
    void UI();

    const std::string NAME = "VideoCapture";

  private:
    /// Open video capture device
    bool openDevice();

    /// Configure input stream
    bool configIStream();

    /// Configure output stream
    bool configOStream(AVFormatContext *oc);

    /// Decode frame from input stream packet
    bool decode(AVCodecContext *codecCtx, AVFrame *frame, AVPacket *pkt);

    /// Encode frame to output stream packet
    bool encode(AVCodecContext *codecCtx, AVFrame *frame, AVPacket *pkt);

    std::unique_ptr<InputStream> _ist;
    std::unique_ptr<OutputStream> _ost;

    // x, y, w, h, fps, bitrate
    std::array<int, 6> _configs;
    bool _autoBitrate;
};
