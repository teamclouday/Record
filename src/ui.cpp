#include <imgui.h>

#include "audiocapture.hpp"
#include "context.hpp"
#include "media.hpp"
#include "videocapture.hpp"

#include <string>

void AppContext::UI()
{
    ImGui::Text("Window Size: %dx%d", _winWidth, _winHeight);
    ImGui::Text("Window Pos: (%d, %d)", _winPosX, _winPosY);
    ImGui::Text("Window FPS: %.2f", ImGui::GetIO().Framerate);
    ImGui::Text("Screen Resolution: %dx%d", _monWidth, _monHeight);
    ImGui::DragFloat("Transparency", &_alpha, 0.01f, 0.0f, 1.0f, "%.2f");
    ImGui::DragInt("Border Width", &_borderNumPixels, 1, 1, 50);
    ImGui::ColorEdit3("Border Color", _borderColor.data());
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Control"))
    {
        if (ImGui::TreeNode("ESC"))
        {
            ImGui::Text("Exit Application");
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("F11"))
        {
            ImGui::Text("Maximize Window");
            ImGui::TreePop();
        }
        auto hotkey = "CTRL+F" + std::to_string(_hotkeyNum);
        if (ImGui::TreeNode(hotkey.c_str()))
        {
            ImGui::Text("Toggle Recording");
            ImGui::TreePop();
        }
    }
}

void MediaHandler::UI()
{
    ImGui::Text("Output File Path:");
    ImGui::TextWrapped(_media->path.c_str());
    if (ImGui::Button("Set File"))
        SelectOutputPath();
    ImGui::DragInt("Skip Time (ms)", &_media->skipTime, 10, 0, 10000);
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Video"))
    {
        _video->UI();
    }
    if (_media->canAudio && ImGui::CollapsingHeader("Audio"))
    {
        _audio->UI();
    }
}

void VideoCapture::UI()
{
    ImGui::DragInt("FPS", &_configs[4], 5, 5, 60);
    ImGui::Checkbox("Auto Bit Rate", &_autoBitRate);
    if (!_autoBitRate)
        ImGui::DragInt("Bit Rate", &_configs[5], 10000, 10000, 10000000);
}

void AudioCapture::UI()
{
    ImGui::DragInt("Sample Rate", &_sampleRate, 100, 8000, 100000);
    ImGui::Checkbox("Auto Bit Rate", &_autoBitRate);
    if (!_autoBitRate)
        ImGui::DragInt("Bit Rate", &_bitRate, 100, 100, 400000);
    ImGui::Checkbox("Capture Audio", &_captureOut);
    ImGui::Checkbox("Capture Mic", &_captureMic);
#if __linux__
    if (_captureOut)
    {
        ImGui::Separator();
        if (ImGui::TreeNode("Audio Devices"))
        {
            for (auto &data : _pulse->outDevices)
            {
                ImGui::PushID(data.second);
                ImGui::RadioButton(("ID = " + std::to_string(data.second)).c_str(), &_pulse->outIdx, data.second);
                ImGui::Indent(50.0f);
                ImGui::TextWrapped(data.first.c_str());
                ImGui::Unindent(50.0f);
                ImGui::PopID();
            }
            ImGui::TreePop();
        }
    }
    if (_captureMic)
    {
        ImGui::Separator();
        if (ImGui::TreeNode("Mic Devices"))
        {
            for (auto &data : _pulse->micDevices)
            {
                ImGui::PushID(data.second);
                ImGui::RadioButton(("ID = " + std::to_string(data.second)).c_str(), &_pulse->micIdx, data.second);
                ImGui::Indent(50.0f);
                ImGui::TextWrapped(data.first.c_str());
                ImGui::Unindent(50.0f);
                ImGui::PopID();
            }
            ImGui::TreePop();
        }
    }
    if (ImGui::Button("Refresh Devices"))
        _pulse->refresh();
#endif
}
