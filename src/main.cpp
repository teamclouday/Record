#include "media.hpp"
#include "context.hpp"
#include "utils.hpp"
#include <imgui.h>
#include <memory>
#include <string>

int main()
{
    std::shared_ptr<AppContext> ctx;
    std::shared_ptr<MediaHandler> handler;
    const std::string appname = "Recorder";

    // init variables
    try
    {
        ctx = std::make_shared<AppContext>(appname);
        handler = std::make_shared<MediaHandler>();
    }catch(const std::exception& e)
    {
        display_message(appname, e.what(), MESSAGE_ERROR);
        display_message(appname, "application quits", MESSAGE_INFO);
        return -1;
    }

    // define custom UI function
    auto renderUI = [&]()
    {
        auto& io = ImGui::GetIO();
        ImGui::SetNextWindowPos({io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f}, ImGuiCond_Always, {0.5f,0.5f});
        ImGui::SetNextWindowSize({io.DisplaySize.x * 0.8f, io.DisplaySize.y * 0.8f}, ImGuiCond_Always);
        ImGui::Begin("UI", nullptr, ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize);
        if(ImGui::BeginTabBar("Configs"))
        {
            if(ImGui::BeginTabItem("Context"))
            {
                ctx->UI();
                ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("Video"))
            {
                handler->UI();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
    };

    // attach handler
    ctx->AttachHandler(handler);
    // start app loop
    ctx->AppLoop(renderUI);

    return 0;
}