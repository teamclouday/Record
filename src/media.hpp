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

#define VIDEO_DELAY_SECONDS     0
#define VIDEO_DEFAULT_FPS       30
#define VIDEO_DEFAULT_BITRATE   4000000
#define VIDEO_DEFAULT_OUTPUT    "out.mp4"

class MediaHandler
{
public:
    MediaHandler();
    ~MediaHandler();

    void ConfigWindow(int x, int y, int w, int h);
    bool StartRecord();
    bool StopRecord();
    void SelectOutputPath();
    bool IsRecording();

    void UI();

    const std::string NAME = "MediaHandler";

private:
    bool openCapture();
    bool prepareParams();
    void freeLibAV();
    void recordInternal();
    bool decodeVideo(AVCodecContext* icodecCtx, AVFrame* iframe, AVPacket* ipacket);
    bool encodeVideo(AVCodecContext* ocodecCtx, AVFrame* oframe, AVPacket* opacket);
    void validateOutputFormat();

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