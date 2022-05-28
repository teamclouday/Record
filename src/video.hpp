#pragma once
extern "C"
{
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libavcodec/avcodec.h>
}
#include <string>

/** @file */

class VideoHandler
{
public:
    VideoHandler();
    ~VideoHandler();

    void ConfigWindow(int x, int y, int w, int h);
    void StartRecord();
    void StopRecord();

    void UI();

private:
    void openCapture();
    void setOutputPath();
    void setRecordParams();

    // record size
    int _rX, _rY, _rW, _rH;
    float _fps;
    int _bitRate;
    std::string _outFilePath;
    // libav variables
    AVFormatContext* _ifmtCtx, * _ofmtCtx;
    AVCodecParameters* _icodecParams, * _ocodecParams;
    AVCodec* _icodec, * _ocodec;
    AVCodecContext* _icodecCtx, * _ocodecCtx;
    AVFrame* _iAVFrame,*  _oAVFrame;
    AVDictionary* _options;
    AVPacket* _packet;
    AVStream* _stream;
};