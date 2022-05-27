#pragma once

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

class VideoHandler
{
public:
    VideoHandler();
    ~VideoHandler();

    void UI();

private:
};