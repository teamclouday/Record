#include "context.hpp"
#include "utils.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stdexcept>
#include <sstream>
#include <cstring>

AppContext::AppContext(const std::string& title)
{
    _displayUI = true;
    _alpha = 0.75f;
    // init window
    _winWidth = 800;
    _winHeight = 600;
    _title = title;
    if(!glfwInit())
        throw std::runtime_error("Failed to init GLFW!");
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    _window = glfwCreateWindow(_winWidth, _winHeight, _title.c_str(), nullptr, nullptr);
    if(!_window)
        throw std::runtime_error("Failed to create GLFW window!");
    glfwSetWindowSizeLimits(_window, WIN_MIN_WIDTH, WIN_MIN_HEIGHT, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwSetWindowUserPointer(_window, this);
    glfwSetKeyCallback(_window, glfw_key_callback);
    glfwSetWindowPosCallback(_window, glfw_windowpos_callback);
    glfwSetWindowSizeCallback(_window, glfw_windowsize_callback);
    glfwMakeContextCurrent(_window);
    glfwSwapInterval(1);
    glfwGetWindowPos(_window, &_winPosX, &_winPosY);
    glfwGetWindowSize(_window, &_winWidth, &_winHeight);
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
    fontConfig.SizePixels = 20.0f;
    fontConfig.OversampleH = fontConfig.OversampleV = 1;
    fontConfig.PixelSnapH = true;
    io.Fonts->AddFontDefault(&fontConfig);
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(_window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    // prepare border shader
    prepareBorder();
}

AppContext::~AppContext()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(_window);
    glfwTerminate();
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
            case GLFW_KEY_F10: // toggle UI and start/stop recording
            {
                bool success = true;
                user->toggleUI();
                // start/stop recording
                if(user->_displayUI && user->_mediaHandler)
                    success = user->_mediaHandler->StopRecord();
                else if(!user->_displayUI && user->_mediaHandler)
                {
                    user->_mediaHandler->ConfigWindow(
                        user->_winPosX + WIN_BORDER_PIXELS,
                        user->_winPosY + WIN_BORDER_PIXELS,
                        user->_winWidth - WIN_BORDER_PIXELS * 2,
                        user->_winHeight - WIN_BORDER_PIXELS * 2);
                    success = user->_mediaHandler->StartRecord();
                }
                if(!success) user->toggleUI();
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

void AppContext::prepareBorder()
{
    int success;
    constexpr int infoLogSize = 512;
    char infoLog[infoLogSize];
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
        glGetShaderInfoLog(vertShader, infoLogSize, NULL, infoLog);
        std::stringstream sstr;
        sstr << "failed to compile shader (" << infoLog << ")";
        display_message(NAME, sstr.str(), MESSAGE_WARN);
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
        std::memset(infoLog, 0, infoLogSize);
        glGetShaderInfoLog(fragShader, infoLogSize, NULL, infoLog);
        std::stringstream sstr;
        sstr << "failed to compile shader (" << infoLog << ")";
        display_message(NAME, sstr.str(), MESSAGE_WARN);
    }
    _borderProg =  glCreateProgram();
    glAttachShader(_borderProg, vertShader);
    glAttachShader(_borderProg, fragShader);
    glLinkProgram(_borderProg);
    glGetProgramiv(_borderProg, GL_LINK_STATUS, &success);
    if(!success)
    {
        std::memset(infoLog, 0, infoLogSize);
        glGetProgramInfoLog(_borderProg, 512, NULL, infoLog);
        std::stringstream sstr;
        sstr << "failed to link program (" << infoLog << ")";
        display_message(NAME, sstr.str(), MESSAGE_WARN);
    }
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
}

void AppContext::toggleUI()
{
    _displayUI = !_displayUI;
    if(_displayUI)
    {
        glfwSetWindowAttrib(_window, GLFW_MOUSE_PASSTHROUGH, GLFW_FALSE);
        glfwSetWindowAttrib(_window, GLFW_DECORATED, GLFW_TRUE);
    }
    else
    {
        glfwSetWindowAttrib(_window, GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);
        glfwSetWindowAttrib(_window, GLFW_DECORATED, GLFW_FALSE);
    }
}

void AppContext::AttachHandler(std::shared_ptr<MediaHandler> handler)
{
    _mediaHandler = handler;
}

void AppContext::AppLoop(std::function<void()> customUI)
{
    while(!glfwWindowShouldClose(_window))
    {
        // set render area
        glViewport(0, 0, _winWidth, _winHeight);
        glClearColor(0.0f, 0.0f, 0.0f, _displayUI ? _alpha : 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        // prepare UI draw
        if(_displayUI && customUI)
        {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            customUI();
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }
        else if(!_displayUI)
        {
            glUseProgram(_borderProg);
            glUniform1f(glGetUniformLocation(_borderProg, "dW"), WIN_BORDER_PIXELS / static_cast<float>(_winWidth));
            glUniform1f(glGetUniformLocation(_borderProg, "dH"), WIN_BORDER_PIXELS / static_cast<float>(_winHeight));
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glUseProgram(0);
        }
        // refresh frame
        glfwSwapBuffers(_window);
        // poll events
        glfwPollEvents();
    }
}