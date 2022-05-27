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
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    // glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);
    // glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    _window = glfwCreateWindow(_winWidth, _winHeight, _title.c_str(), nullptr, nullptr);
    if(!_window)
        throw std::runtime_error("Failed to create GLFW window!");
    glfwSetWindowSizeLimits(_window, WIN_MIN_WIDTH, WIN_MIN_HEIGHT, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwSetWindowUserPointer(_window, this);
    glfwSetKeyCallback(_window, glfw_key_callback);
    glfwSetWindowPosCallback(_window, glfw_windowpos_callback);
    glfwSetWindowSizeCallback(_window, glfw_windowsize_callback);
    glfwMakeContextCurrent(_window);
    // init opengl context
    glewExperimental = GL_TRUE;
    if(glewInit() != GLEW_OK)
        throw std::runtime_error("Failed to init GLEW!");
    // init imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    ImFontConfig fontConfig;
    fontConfig.SizePixels = 25.0f;
    fontConfig.OversampleH = fontConfig.OversampleV = 1;
    fontConfig.PixelSnapH = true;
    io.Fonts->AddFontDefault(&fontConfig);
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(_window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    // prepare outline shader
    prepareOutline();
}

AppContext::~AppContext()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(_window);
    glfwTerminate();
}

bool AppContext::CanLoop()
{
    return !glfwWindowShouldClose(_window);
}

void AppContext::BeginFrame()
{
    glViewport(0, 0, _winWidth, _winHeight);
    glClearColor(0.0f, 0.0f, 0.0f, _displayUI ? 0.6f : 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    _timer = glfwGetTime();
}

void AppContext::EndFrame(std::function<void()> customUI)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    if(_displayUI && customUI)
    {
        customUI();
    }
    else if(!_displayUI)
    {
        glUseProgram(_outlineProg);
        glUniform1f(glGetUniformLocation(_outlineProg, "dW"), WIN_BORDER_PIXELS / static_cast<float>(_winWidth));
        glUniform1f(glGetUniformLocation(_outlineProg, "dH"), WIN_BORDER_PIXELS / static_cast<float>(_winHeight));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glUseProgram(0);
    }
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(_window);
    glfwPollEvents();
    // fps control
    double elapsed = glfwGetTime() - _timer;
    if(elapsed < _spf)
        std::this_thread::sleep_for(std::chrono::duration<double>(_spf - elapsed));
}

void AppContext::SetFPS(double fps)
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
            case GLFW_KEY_F10: // toggle UI
            {
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
}

void AppContext::glfw_windowsize_callback(GLFWwindow* window, int w, int h)
{
    AppContext* user = reinterpret_cast<AppContext*>(glfwGetWindowUserPointer(window));
    user->_winWidth = w;
    user->_winHeight = h;
}

void AppContext::glfw_windowpos_callback(GLFWwindow* window, int x, int y)
{
    AppContext* user = reinterpret_cast<AppContext*>(glfwGetWindowUserPointer(window));
    user->_winPosX = x;
    user->_winPosY = y;
}

void AppContext::prepareOutline()
{
    // TODO: clean std::cout code
    int success;
    char infoLog[512];
    const char* vertSource =
    "#version 330 core\n\
     varying vec2 coord;\
     const vec2 data[4] = vec2[](vec2(-1.0,-1.0),vec2(1.0,-1.0),vec2(-1.0,1.0),vec2(1.0,1.0));\
     void main()\
     {\
        coord = data[gl_VertexID] * 0.5 + 0.5;\
        gl_Position = vec4(data[gl_VertexID], 0.0, 1.0);\
     }\
    ";
    auto vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, &vertSource, nullptr);
    glCompileShader(vertShader);
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(vertShader, 512, NULL, infoLog);
        std::cout << infoLog << std::endl;
    }
    const char* fragSource =
    "#version 330 core\n\
     varying vec2 coord;\
     uniform float dW;\
     uniform float dH;\
     void main()\
     {\
        vec2 w = vec2(dW, 1.0 - dW);\
        vec2 h = vec2(dH, 1.0 - dH);\
        if(coord.x < w.y && coord.x > w.x &&\
           coord.y < h.y && coord.y > h.x)\
           {discard;}\
        else\
            {gl_FragColor = vec4(1,0.5,0.5,1.0);}\
     }\
    ";
    auto fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &fragSource, nullptr);
    glCompileShader(fragShader);
    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(fragShader, 512, NULL, infoLog);
        std::cout << infoLog << std::endl;
    }
    _outlineProg =  glCreateProgram();
    glAttachShader(_outlineProg, vertShader);
    glAttachShader(_outlineProg, fragShader);
    glLinkProgram(_outlineProg);
    glGetProgramiv(_outlineProg, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(_outlineProg, 512, NULL, infoLog);
        std::cout << infoLog << std::endl;
    }
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
}

bool AppContext::IsUIDisplay()
{
    return _displayUI;
}