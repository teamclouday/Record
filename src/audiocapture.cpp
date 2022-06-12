#include "audiocapture.hpp"
#include "utils.hpp"

// reference: https://github.com/cdemoulins/pamixer/blob/master/src/pulseaudio.cc

AudioCapture::AudioCapture()
{
}

AudioCapture::~AudioCapture()
{
}

bool AudioCapture::openCapture(AVFormatContext *oc)
{
    return true;
}

bool AudioCapture::closeCapture()
{
    return true;
}

bool AudioCapture::writeFrame(AVFormatContext *oc, bool skip)
{
    return true;
}

bool AudioCapture::openCaptureDevice()
{
    return true;
}

bool AudioCapture::configInputStream()
{
    return true;
}

bool AudioCapture::configOutputStream(AVFormatContext *oc)
{
    return true;
}
