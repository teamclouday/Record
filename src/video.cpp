#include "video.hpp"
#include "utils.hpp"
#include <stdexcept>

// reference: https://github.com/leandromoreira/ffmpeg-libav-tutorial
// reference: https://github.com/abdullahfarwees/screen-recorder-ffmpeg-cpp/blob/master/src/ScreenRecorder.cpp
// reference: https://stackoverflow.com/questions/70390402/why-ffmpeg-screen-recorder-output-shows-green-screen-only

VideoHandler::VideoHandler()
{
    // default values
    _rX = 10;
    _rY = 10;
    _rW = 800;
    _rH = 600;
    _fps = 30.0f;
    _bitRate = 40000;
    _outFilePath = "out.mp4";
    // start libav
    avdevice_register_all();
    _ifmtCtx = avformat_alloc_context();
    _ofmtCtx = avformat_alloc_context();
    _icodecParams = avcodec_parameters_alloc();
    _ocodecParams = avcodec_parameters_alloc();
}

VideoHandler::~VideoHandler()
{
    avformat_free_context(_ifmtCtx);
    avformat_free_context(_ofmtCtx);
    avcodec_parameters_free(&_icodecParams);
    avcodec_parameters_free(&_ocodecParams);
}

void VideoHandler::openCapture()
{
    std::string captureSource = "";
    // check if screen capture is supported
    // platform specific code
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    // TODO: implement code for windows
#elif __linux__
    // by default assume X11 backend (wayland won't work)
    captureSource = "x11grab";
#else
    throw std::runtime_error("Unsupported Platform!");
#endif
    auto format = av_find_input_format(captureSource.c_str());
    if(0 != avformat_open_input(&_ifmtCtx, nullptr, format, &_options))
        throw std::runtime_error("Failed to Open " + captureSource + "!");
}

void VideoHandler::setOutputPath()
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    // TODO: implement code for windows
#elif __linux__
    
#else
    // unsupported platform
    _outFilePath = "out.mp4";
#endif
}

void VideoHandler::setRecordParams()
{

}

void VideoHandler::ConfigWindow(int x, int y, int w, int h)
{
    _rX = x;
    _rY = y;
    _rW = w;
    _rH = h;
}

void VideoHandler::StartRecord()
{

}

void VideoHandler::StopRecord()
{

}