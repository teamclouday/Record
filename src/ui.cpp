#include "context.hpp"
#include "video.hpp"
#include <imgui.h>

void AppContext::UI()
{
    ImGui::Text("Window Size: %dx%d", _winWidth, _winHeight);
    ImGui::Text("Window Pos: (%d, %d)", _winPosX, _winPosY);
    ImGui::Text("FPS: %.2f", ImGui::GetIO().Framerate);
}

void VideoHandler::UI()
{
    
}