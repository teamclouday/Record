#pragma once
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
}

#include "audiocapture.hpp"
#include "videocapture.hpp"

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>

namespace fs = std::filesystem;

/** @file */

/// Video skip milliseconds at beginning
#define OUTPUT_SKIP_TIME 1000

/// Video default output path
#define OUTPUT_PATH_DEFAULT "out.mp4"

/**
 * @brief Media Output
 *
 * This structure stores media output info.
 */
struct MediaOutput
{
    AVFormatContext *fmtCtx;
    int32_t x, y, w, h;
    int32_t skipTime;
    std::string path;
    bool canAudio;

    MediaOutput() : fmtCtx(nullptr), x(0), y(0), w(0), h(0), skipTime(OUTPUT_SKIP_TIME), canAudio(true)
    {
        setPath(OUTPUT_PATH_DEFAULT);
    }

    ~MediaOutput()
    {
        if (fmtCtx)
            avformat_free_context(fmtCtx);
    }

    /**
     * @brief Set Output File to Absolute Path
     *
     * @param newPath New output path
     */
    void setPath(const std::string &newPath)
    {
        path = fs::absolute(newPath).string();
    }
};

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
    /// Internal record process
    void recordInternal();

    /// Validate selected output file format
    void validateOutputFormat();

    /// Try to lock output file
    bool lockMediaFile();

    /// Unlock file after recording stop
    void unlockMediaFile();

    /// Init media output parameters
    bool initMedia();

    /// Open media file and write header
    bool openMedia();

    /// Close media file
    bool closeMedia();

    std::unique_ptr<VideoCapture> _video;
    std::unique_ptr<AudioCapture> _audio;
    std::unique_ptr<MediaOutput> _media;

    // record thread configs
    bool _recording;
    bool _recordLoop;
    std::thread _recordT;
};
