#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <Windows.h>
#include <commdlg.h>
#endif

#include "media.hpp"
#include "utils.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace fs = std::filesystem;

// reference: https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/muxing.c

MediaHandler::MediaHandler() : _recording(false)
{
    _media = std::make_unique<MediaOutput>();
    initMedia();
    _video = std::make_unique<VideoCapture>();
    _audio = std::make_unique<AudioCapture>();
    avdevice_register_all();
}

MediaHandler::~MediaHandler()
{
    StopRecord();
}

void MediaHandler::ConfigWindow(int x, int y, int w, int h, int mw, int mh)
{
    int xx = (std::max)(0, (std::min)(mw, x + w));
    int yy = (std::max)(0, (std::min)(mh, y + h));
    _media->x = (std::max)(0, (std::min)(mw, x));
    _media->y = (std::max)(0, (std::min)(mh, y));
    _media->w = xx - _media->x;
    _media->h = yy - _media->y;
    if (_media->x != x || _media->y != y || _media->w != w || _media->h != h)
    {
        display_message(NAME,
                        "capture area changed to (" + std::to_string(_media->x) + "," + std::to_string(_media->y) +
                            "|" + std::to_string(_media->w) + "x" + std::to_string(_media->h) + ")",
                        MESSAGE_WARN);
    }
    // fix for H264
    if (_media->h % 2)
        _media->h--;
}

bool MediaHandler::StartRecord()
{
    StopRecord();
    // av_log_set_level(AV_LOG_DEBUG);
    bool success = true;
    // try to lock output file
    success = success && lockMediaFile();
    // init video
    success = success && _video->openCapture(_media->fmtCtx, {_media->x, _media->y, _media->w, _media->h});
    // init audio
    success = success && (!_media->canAudio || _audio->openCapture(_media->fmtCtx));
    // open file
    success = success && openMedia();
    if (!success)
        return false;
    // start thread
    _recordLoop = true;
    _recordT = std::thread([this] { recordInternal(); });
    return true;
}

bool MediaHandler::StopRecord()
{
    _recordLoop = false;
    if (_recordT.joinable())
        _recordT.join();
    _video->closeCapture();
    _audio->closeCapture();
    return true;
}

void MediaHandler::SelectOutputPath()
{
    _media = std::make_unique<MediaOutput>();
    char filepath[1025] = "out";
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    // TODO: implement code for windows
    filepath[0] = '\0';
    OPENFILENAMEA f{};
    f.lStructSize = sizeof(OPENFILENAMEA);
    f.hwndOwner = NULL;
    f.lpstrFile = filepath;
    f.lpstrFilter = "MP4\0*.mp4\0GIF\0*.gif\0WEBM\0*.webm\0MOV\0*.mov\0WMV\0*.wmv\0AVI\0*.avi\0FLV\0*.flv\0APNG\0*."
                    "apng\0MPG\0*.mpg\0All Files\0*.*\0\0";
    f.lpstrTitle = "Set Output File";
    f.nMaxFile = 1024;
    f.lpstrDefExt = "mp4";
    f.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
    GetOpenFileNameA(&f);
#elif __linux__
    FILE *f = popen("zenity --file-selection --save --confirm-overwrite\
        --title=\"Set Output File\"\
        --filename=\"out.mp4\"",
                    "r");
    fgets(filepath, 1024, f);
    pclose(f);
    filepath[strlen(filepath) - 1] = 0;
#else
    // unsupported platform
    _media->path = fs::absolute(OUTPUT_PATH_DEFAULT).string();
    display_message(NAME, "unsupported capture platform!", MESSAGE_WARN);
#endif
    _media->path = std::string(filepath);
    validateOutputFormat();
    if (!initMedia())
    {
        _media = std::make_unique<MediaOutput>();
        initMedia();
    }
}

bool MediaHandler::IsRecording()
{
    return _recording;
}

void MediaHandler::recordInternal()
{
    _recording = true;
    // delay
    display_message(NAME, "started recording", MESSAGE_INFO);
    // start reading frames
    int64_t frameCount = -_media->framesSkip;
    bool videoRead = true, audioRead = true, skip;
    do
    {
        skip = frameCount < 0;
        videoRead = _video->writeFrame(_media->fmtCtx, skip);
        audioRead = _audio->writeFrame(_media->fmtCtx, skip);
        if (skip)
            frameCount++;
    } while ((videoRead || audioRead) && _recordLoop);
    closeMedia();
    display_message(NAME, "stopped recording", MESSAGE_INFO);
    display_message(NAME, "output saved to " + _media->path, MESSAGE_INFO);
    unlockMediaFile();
    _recording = false;
}

void MediaHandler::validateOutputFormat()
{
    const std::vector<std::string> SUPPORT_EXTS = {".mp4", ".mov", ".wmv",  ".gif", ".webm",
                                                   ".avi", ".flv", ".apng", ".mpg"};
    auto ext = fs::path(_media->path).extension().string();
    for (auto &sup : SUPPORT_EXTS)
    {
        if (!ext.compare(sup))
            return;
    }
    display_message(NAME, "unsupported output format " + ext + ", setting to default", MESSAGE_WARN);
    _media->path = fs::absolute(OUTPUT_PATH_DEFAULT).string();
}

bool MediaHandler::lockMediaFile()
{
    auto lockFileName = _media->path + ".lock";
    // check if lock exists
    if (fs::exists(lockFileName))
    {
        display_message(
            NAME, "media file is locked by another program. If not, delete \"" + lockFileName + "\" and try again",
            MESSAGE_WARN);
        return false;
    }
    // create lock file
    std::fstream f(lockFileName, std::ios::out);
    if (!f.is_open())
    {
        display_message(NAME, "failed to lock media file", MESSAGE_WARN);
        return false;
    }
    f.close();
    return true;
}

void MediaHandler::unlockMediaFile()
{
    auto lockFileName = _media->path + ".lock";
    fs::remove(lockFileName);
}

bool MediaHandler::initMedia()
{
    auto formatOut = av_guess_format(nullptr, _media->path.c_str(), nullptr);
    // allocate format
    {
        if (!formatOut)
        {
            display_message(NAME, "failed to guess format for " + _media->path, MESSAGE_WARN);
            return false;
        }
        if (avformat_alloc_output_context2(&_media->fmtCtx, formatOut, nullptr, _media->path.c_str()) < 0)
        {
            display_message(NAME, "failed to allocate format for " + _media->path, MESSAGE_WARN);
            return false;
        }
        _media->fmtCtx->video_codec_id = formatOut->video_codec;
        _media->fmtCtx->audio_codec_id = formatOut->audio_codec;
    }
    // check video & audio
    {
        if (formatOut->video_codec == AV_CODEC_ID_NONE)
        {
            display_message(NAME, "output file does not support video", MESSAGE_WARN);
            return false;
        }
        if (formatOut->audio_codec == AV_CODEC_ID_NONE)
        {
            _media->canAudio = false;
        }
    }
    // config output
    {
        if (formatOut->video_codec == AV_CODEC_ID_APNG)
        {
            av_opt_set_int(_media->fmtCtx->priv_data, "plays", 0, 0);
        }
    }
    return true;
}

bool MediaHandler::openMedia()
{
    if (avio_open(&_media->fmtCtx->pb, _media->path.c_str(), AVIO_FLAG_WRITE) < 0)
    {
        display_message(NAME, "failed to open " + _media->path, MESSAGE_WARN);
        return false;
    }
    if (avformat_write_header(_media->fmtCtx, nullptr) < 0)
    {
        display_message(NAME, "failed to write header to " + _media->path, MESSAGE_WARN);
        return false;
    }
    av_dump_format(_media->fmtCtx, 0, _media->path.c_str(), 1);
    return true;
}

bool MediaHandler::closeMedia()
{
    if (av_write_trailer(_media->fmtCtx) != 0)
        display_message(NAME, "failed to write trailer to " + _media->path, MESSAGE_WARN);
    return true;
}
