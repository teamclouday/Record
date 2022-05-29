#include "context.hpp"
#include "media.hpp"
#include <imgui.h>

void AppContext::UI()
{
    ImGui::Text("Window Size: %dx%d", _winWidth, _winHeight);
    ImGui::Text("Window Pos: (%d, %d)", _winPosX, _winPosY);
    ImGui::Text("FPS: %.2f", ImGui::GetIO().Framerate);
    ImGui::DragFloat("Transparency", &_alpha, 0.01f, 0.0f, 1.0f, "%.2f");
    ImGui::Separator();
    if(ImGui::CollapsingHeader("Control"))
    {
        if(ImGui::TreeNode("ESC")){ImGui::Text("Exit Application"); ImGui::TreePop();}
        if(ImGui::TreeNode("F11")){ImGui::Text("Maximize Window"); ImGui::TreePop();}
        if(ImGui::TreeNode("CTRL+F10")){ImGui::Text("Toggle Recording"); ImGui::TreePop();}
    }
}

void MediaHandler::UI()
{
    ImGui::Text("Output File Path:");
    ImGui::TextWrapped(_outFilePath.c_str());
    if(ImGui::Button("Set File"))
        SelectOutputPath();
    ImGui::DragInt("FPS", &_fps, 5, 5, 60);
    ImGui::DragInt("Delay (s)", &_delaySeconds, 1, 0, 5);
    ImGui::Checkbox("Auto Bit Rate", &_bitrateAuto);
    if(!_bitrateAuto) ImGui::DragInt("Bit Rate", &_bitrate, 10000, 10000, 10000000);
    ImGui::Separator();
}