#pragma once
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}

#include "streams.hpp"

#include <array>
#include <memory>
#include <string>

class AudioCapture
{
  public:
    AudioCapture();
    ~AudioCapture();

    bool openCapture(AVFormatContext *oc);
    bool closeCapture();
    bool writeFrame(AVFormatContext *oc, bool skip);

    void UI();

    const std::string NAME = "AudioCapture";

  private:
    bool openCaptureDevice();
    bool configInputStream();
    bool configOutputStream(AVFormatContext *oc);

    std::unique_ptr<InputStream> _ist;
    std::unique_ptr<OutputStream> _ost;
};
