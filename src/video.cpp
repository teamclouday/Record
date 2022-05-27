#include "video.hpp"
#include <stdexcept>

// reference: https://github.com/leandromoreira/ffmpeg-libav-tutorial
// reference: https://github.com/abdullahfarwees/screen-recorder-ffmpeg-cpp/blob/master/src/ScreenRecorder.cpp

VideoHandler::VideoHandler()
{
    openCapture();
}

VideoHandler::~VideoHandler()
{
    
}

void VideoHandler::openCapture()
{
    // check if screen capture is supported
    // platform specific code
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    // TODO: implement code to check
#elif __linux__
    // by default assume X11 backend (wayland won't work)
    av_find_input_format("x11grab");
#else
    throw std::runtime_error("Unsupported Platform!");
#endif
}