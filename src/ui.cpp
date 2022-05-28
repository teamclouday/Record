#include "context.hpp"
#include "media.hpp"
#include <imgui.h>

void AppContext::UI()
{
    ImGui::Text("Window Size: %dx%d", _winWidth, _winHeight);
    ImGui::Text("Window Pos: (%d, %d)", _winPosX, _winPosY);
    ImGui::Text("FPS: %.2f", ImGui::GetIO().Framerate);
    ImGui::DragFloat("Transparency", &_alpha, 0.01f, 0.0f, 1.0f, "%.2f");
}

void MediaHandler::UI()
{
    
}

void MediaHandler::UIAudio()
{

}