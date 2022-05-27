#include "video.hpp"
#include "context.hpp"
#include "utils.hpp"
#include <imgui.h>
#include <memory>
#include <string>

int main()
{
    std::shared_ptr<AppContext> ctx;
    std::shared_ptr<VideoHandler> handler;
    const std::string appname = "Recorder";

    auto renderUI = [&]()
    {
        ImGui::SetNextWindowSize({300.0f, 200.0f}, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos({10.0f, 10.0f}, ImGuiCond_FirstUseEver);
        ImGui::Begin("UI");
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

    try
    {
        ctx = std::make_shared<AppContext>(appname);
        handler = std::make_shared<VideoHandler>();
    }catch(const std::exception& e)
    {
        show_fatal_error(e.what(), appname);
        return -1;
    }

    while(ctx->canLoop())
    {
        ctx->beginFrame();

        ctx->endFrame(renderUI);
    }

    return 0;
}