#include "context.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stdexcept>
#include <thread>
#include <chrono>

#include <iostream>

AppContext::AppContext(const std::string& title)
{
    _timer = 0.0;
    _spf = 1.0 / DEFAULT_FPS;
    _displayUI = true;
    // init window
    _winWidth = 800;
    _winHeight = 600;
    _title = title;
    if(!glfwInit())
        throw std::runtime_error("Failed to init GLFW!");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    // glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);
    // glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    _window = glfwCreateWindow(_winWidth, _winHeight, _title.c_str(), nullptr, nullptr);
    if(!_window)
        throw std::runtime_error("Failed to create GLFW window!");
    glfwSetWindowUserPointer(_window, this);
    glfwMakeContextCurrent(_window);
    glfwSetKeyCallback(_window, glfw_key_callback);
    // init opengl context
    glewExperimental = GL_TRUE;
    if(glewInit() != GLEW_OK)
        throw std::runtime_error("Failed to init GLEW!");
    glEnable(GL_TEXTURE_2D);
    // init imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(_window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
}

AppContext::~AppContext()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(_window);
    glfwTerminate();
}

bool AppContext::canLoop()
{
    return !glfwWindowShouldClose(_window);
}

void AppContext::beginFrame()
{
    glfwPollEvents();
    glfwGetFramebufferSize(_window, &_winWidth, &_winHeight);
    glViewport(0, 0, _winWidth, _winHeight);
    glClearColor(0.0f, 0.0f, 0.0f, _displayUI ? 0.6f : 0.15f);
    glClear(GL_COLOR_BUFFER_BIT);
    _timer = glfwGetTime();
}

void AppContext::endFrame(std::function<void()> customUI)
{
    if(_displayUI && customUI)
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        customUI();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
    glfwSwapBuffers(_window);
    // fps control
    double elapsed = glfwGetTime() - _timer;
    if(elapsed < _spf)
        std::this_thread::sleep_for(std::chrono::duration<double>(_spf - elapsed));
}

void AppContext::setFPS(double fps)
{
    _spf = 1.0 / fps;
}

void AppContext::glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    AppContext* user = reinterpret_cast<AppContext*>(glfwGetWindowUserPointer(window));
    if(action == GLFW_PRESS)
    {
        switch(key)
        {
            case GLFW_KEY_ESCAPE: // close window
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                break;
            case GLFW_KEY_F11: // toggle full screen
            {
                if(glfwGetWindowMonitor(window))
                {
                    glfwSetWindowMonitor(
                        window, nullptr, 
                        user->_windowConfig[0], user->_windowConfig[1], 
                        user->_windowConfig[2], user->_windowConfig[3], 0);
                }
                else
                {
                    auto monitor = glfwGetPrimaryMonitor();
                    if(monitor)
                    {
                        glfwGetWindowPos(window, &user->_windowConfig[0], &user->_windowConfig[1]);
                        glfwGetWindowSize(window, &user->_windowConfig[2], &user->_windowConfig[3]);
                        auto mode = glfwGetVideoMode(monitor);
                        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, 0);
                    }
                }
                break;
            }
            case GLFW_KEY_F12: // toggle UI
                user->_displayUI = !user->_displayUI;
                if(user->_displayUI)
                {
                    glfwSetWindowAttrib(user->_window, GLFW_MOUSE_PASSTHROUGH, GLFW_FALSE);
                    glfwSetWindowAttrib(user->_window, GLFW_DECORATED, GLFW_TRUE);
                }
                else
                {
                    glfwSetWindowAttrib(user->_window, GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);
                    glfwSetWindowAttrib(user->_window, GLFW_DECORATED, GLFW_FALSE);
                }
                break;
        }
    }
}