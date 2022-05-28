#pragma once
extern "C"
{
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libavcodec/avcodec.h>
}
#include <string>
#include <thread>

/** @file */

#define VIDEO_DEFAULT_FPS       30
#define VIDEO_DEFAULT_BITRATE   40000
#define VIDEO_DEFAULT_OUTPUT    "out.mp4"

class MediaHandler
{
public:
    MediaHandler();
    ~MediaHandler();

    void ConfigWindow(int x, int y, int w, int h);
    void StartRecord();
    void StopRecord();
    void SelectOutputPath();
    bool IsRecording();

    void UI();
    void UIAudio();

    const std::string NAME = "MediaHandler";

private:
    bool openCapture();
    bool prepareParams();
    void freeLibAV();
    void recordInternal();
    bool decodeVideo(AVCodecContext* icodecCtx, AVFrame* iframe, AVPacket* ipacket);
    bool encodeVideo(AVCodecContext* ocodecCtx, AVFrame* oframe, AVPacket* opacket);

    // record size
    int _rX, _rY, _rW, _rH, _fps;
    int _bitrate;
    bool _recording;
    std::string _outFilePath;

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
};