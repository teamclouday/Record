#pragma once
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libavutil/avutil.h>
}

class VideoHandler
{
public:
    VideoHandler();
    ~VideoHandler();

    void StartRecord();
    void StopRecord();

    void UI();

private:
    void openCapture();



};