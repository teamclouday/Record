#pragma once
extern "C"
{
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}
#include <string>
#include <thread>

/** @file */

/// Video capture start delay in seconds
#define VIDEO_DELAY_SECONDS     0

/// Video capture default FPS
#define VIDEO_DEFAULT_FPS       30

/// Video skip frames at beginning
#define VIDEO_FRAMES_SKIP       10

/// Video capture default bit rate
#define VIDEO_DEFAULT_BITRATE   4000000

/// Video default output path
#define VIDEO_DEFAULT_OUTPUT    "out.mp4"

/**
 * @brief Media Handler
 * 
 * This class handles screen capture and audio recording and saves output to a video file.
 */
class MediaHandler
{
public:
    MediaHandler();
    ~MediaHandler();

    /**
     * @brief Configure Capture Window
     * 
     * Is meant to be called from AppContext.
     * 
     * @param x Top-left corner on monitor X-axis
     * @param y Top-left corner on monitor Y-axis
     * @param w Capture width
     * @param h Capture height
     * @param mw Monitor Width
     * @param mh Monitor height
     */
    void ConfigWindow(int x, int y, int w, int h, int mw, int mh);

    /**
     * @brief Start Recording
     * 
     * Is meant to be called from AppContext.
     * 
     * @return true if success
     * @return false otherwise
     */
    bool StartRecord();

    /**
     * @brief Stop Recording
     * 
     * Is meant to be called from AppContext.
     * 
     * @return true if success
     * @return false otherwise
     */
    bool StopRecord();

    /**
     * @brief Select Output File Path
     * 
     * Is meant to be called from UI.
     */
    void SelectOutputPath();

    /**
     * @brief Is Currently Recording
     * 
     * @return true if recording
     * @return false otherwise
     */
    bool IsRecording();

    /**
     * @brief UI Calls
     * 
     * Is meant to be called in customUI function.
     */
    void UI();

    const std::string NAME = "MediaHandler";

private:
    /// Open screen capture
    bool openCapture();

    /// Prepare capture parameters
    bool prepareParams();

    /// free variables related to LibAV
    void freeLibAV();

    /// Internal record process
    void recordInternal();

    /// Helper function to decode video frame
    bool decodeVideo(AVCodecContext* icodecCtx, AVFrame* iframe, AVPacket* ipacket);

    /// Helper function to encode video frame
    bool encodeVideo(AVCodecContext* ocodecCtx, AVFrame* oframe, AVPacket* opacket);

    /// Validate selected output file format
    void validateOutputFormat();

    /// Try to lock output file
    bool lockMediaFile();

    /// Unlock file after recording stop
    void unlockMediaFile();

    // video settings
    int _rX, _rY, _rW, _rH, _fps;
    int _bitrate, _delaySeconds;
    bool _bitrateAuto;
    // audio settings
    bool _captureAudio;
    bool _captureMic;
    // output file path
    std::string _outFilePath;
    bool _recording;

    bool _recordLoop;
    std::thread _recordT;

    // libav variables
    AVFormatContext* _ifmtCtx = nullptr, * _ofmtCtx = nullptr;
    AVCodecParameters* _icodecParams = nullptr, * _ocodecParams = nullptr;
    AVCodecContext* _icodecCtx = nullptr, * _ocodecCtx = nullptr;
    AVFrame* _iAVFrame = nullptr, *  _oAVFrame = nullptr;
    AVDictionary* _options = nullptr;
    AVPacket* _ipacket = nullptr, * _opacket = nullptr;
    AVStream* _videoStream = nullptr;
    int _videoStreamIdx;
    SwsContext* _swsCtx = nullptr;
};