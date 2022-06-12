#pragma once
#include <pulse/pulseaudio.h>

#include "utils.hpp"

#include <algorithm>
#include <chrono>
#include <functional>
#include <map>
#include <string>
#include <thread>

/** @file */

// reference: https://github.com/cdemoulins/pamixer/blob/master/src/pulseaudio.cc

/**
 * @brief PulseAudio Quert Helper
 *
 * This structure stores necessary info of PulseAudio devices for recording.
 */
struct PulseAudioHelper
{
    int outIdx;
    std::map<std::string, int, std::greater<std::string>> outDevices;
    int micIdx;
    std::map<std::string, int, std::greater<std::string>> micDevices;

    pa_mainloop *_paLoop;
    pa_context *_paCtx;
    bool connected;

    const std::string NAME = "PulseAudioHelper";
    const int MAX_WAIT_TIME = 1000; // milliseconds
    const int SLEEP_TIME = 10;      // milliseconds

    PulseAudioHelper() : outIdx(-1), micIdx(-1)
    {
        connected = false;
        _paLoop = pa_mainloop_new();
        _paCtx = pa_context_new(pa_mainloop_get_api(_paLoop), NAME.c_str());
        pa_context_set_state_callback(_paCtx, &paStateCallback, this);
        if (pa_context_connect(_paCtx, nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0)
        {
            display_message(NAME, "failed to connect to pulse audio server", MESSAGE_WARN);
            return;
        }
        int toWait = MAX_WAIT_TIME;
        while (!connected && toWait > 0)
        {
            if (pa_mainloop_iterate(_paLoop, 0, nullptr) < 0)
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME));
            toWait -= SLEEP_TIME;
        }
        if (!connected)
        {
            display_message(NAME, "connect (timeout)", MESSAGE_WARN);
            return;
        }
        refresh();
    }

    ~PulseAudioHelper()
    {
        if (_paCtx)
        {
            pa_context_set_state_callback(_paCtx, nullptr, nullptr);
            if (connected)
                pa_context_disconnect(_paCtx);
            pa_context_unref(_paCtx);
        }
        if (_paLoop)
            pa_mainloop_free(_paLoop);
    }

    /// Refresh devices for audio capture
    void refresh()
    {
        outIdx = -1;
        micIdx = -1;
        outDevices.clear();
        micDevices.clear();
        // get devices
        {
            auto op = pa_context_get_source_info_list(_paCtx, paSourceCallback, this);
            int toWait = MAX_WAIT_TIME;
            while (pa_operation_get_state(op) == PA_OPERATION_RUNNING && toWait > 0)
            {
                if (pa_mainloop_iterate(_paLoop, 0, nullptr) < 0)
                    break;
                std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME));
                toWait -= SLEEP_TIME;
            }
            if (pa_operation_get_state(op) == PA_OPERATION_RUNNING)
                display_message(NAME, "query sources (timeout)", MESSAGE_WARN);
            pa_operation_unref(op);
        }
    }

    /// PulseAudio source info callback
    static void paSourceCallback(pa_context *ctx, const pa_source_info *info, int eol, void *data)
    {
        if (eol)
            return;
        auto user = reinterpret_cast<PulseAudioHelper *>(data);
        int deviceIdx = info->index;
        auto deviceName = std::string(info->name);
        if (deviceName.find("output") != std::string::npos)
        {
            user->outDevices.insert({deviceName, deviceIdx});
            user->outIdx = (std::max)(deviceIdx, user->outIdx);
        }
        else if (deviceName.find("input") != std::string::npos)
        {
            user->micDevices.insert({deviceName, deviceIdx});
            if (user->micIdx < 0)
                user->micIdx = deviceIdx;
        }
    }

    /// PulseAudio state change callback
    static void paStateCallback(pa_context *ctx, void *data)
    {
        auto user = reinterpret_cast<PulseAudioHelper *>(data);
        if (pa_context_get_state(user->_paCtx) == PA_CONTEXT_READY)
            user->connected = true;
    }
};
