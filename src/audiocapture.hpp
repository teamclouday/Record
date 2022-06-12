#pragma once
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}

#if __linux__
#include "pulsehelper.hpp"
#endif
#include "streams.hpp"

#include <array>
#include <memory>
#include <string>

/** @file */

/// Audio capture default sample rate
#define AUDIO_DEFAULT_SAMPLE_RATE 44100

/// Audio capture default bit rate
#define AUDIO_DEFAULT_BITRATE 64000

/// Audio capture output channels
#define AUDIO_OUTPUT_CHANNELS 2

/**
 * @brief Audio Capture
 *
 * This class handles audio capture, decode & encode.
 * Supports desktop audio & mic audio.
 */
class AudioCapture
{
  public:
    AudioCapture();
    ~AudioCapture();

    /**
     * @brief Open Audio Capture
     *
     * Meant to be called from MediaHandler.
     *
     * @param oc Output format context
     * @return true if starts capture
     * @return false otherwise
     */
    bool openCapture(AVFormatContext *oc);

    /**
     * @brief Close Audio Capture
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
     * @param flush Whether to flush output with empty packets
     * @return true if success
     * @return false otherwise
     */
    bool writeFrame(AVFormatContext *oc, bool skip, bool flush);

    /**
     * @brief UI Calls
     *
     * Is meant to be called from MediaHandler.
     */
    void UI();

    const std::string NAME = "AudioCapture";

  private:
    /// Open audio capture device
    bool openDevice(bool isMic);

    /// Configure input stream
    bool configIStream(InputStream *ist);

    /// Configure audio filter
    bool configFilter();

    /// Configure output stream
    bool configOStream(AVFormatContext *oc);

    /// Decode frame from input stream packet
    bool decode(AVCodecContext *codecCtx, AVFrame *frame, AVPacket *pkt, bool &packetSent);

    /// Encode frame to output stream packet
    bool encode(AVCodecContext *codecCtx, AVFrame *frame, AVPacket *pkt, bool &frameSent);

    /// encode and write packet to output stream
    void writePacket(AVFormatContext *oc, bool skip);

    std::unique_ptr<InputStream> _istOut;
    std::unique_ptr<InputStream> _istMic;
    std::unique_ptr<FilterStream> _filter;
    std::unique_ptr<OutputStream> _ost;

    bool _captureOut, _captureMic, _autoBitRate;
    int _sampleRate, _bitRate;

#if __linux__
    std::unique_ptr<PulseAudioHelper> _pulse;
#endif
};
