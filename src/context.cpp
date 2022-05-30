#include "context.hpp"
#include "utils.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stdexcept>
#include <sstream>
#include <cstring>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <Windows.h>
#define WINHOTKEY_ID    100
#define GLFW_EXPOSE_NATIVE_WIN32
#elif __linux__
#include <X11/Xlib.h>
#include <X11/keysym.h>
#define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3native.h>

extern const int APPICON_W;
extern const int APPICON_H;
extern const std::vector<unsigned char> APPICON_DATA;

AppContext::AppContext(const std::string& title)
{
    _displayUI = true;
    _fullscreen = false;
    _alpha = 0.75f;
    _borderColor = {1.0f,0.5f,0.5f};
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
    glfwSetWindowFocusCallback(_window, glfw_windowfocus_callback);
    glfwMakeContextCurrent(_window);
    glfwSwapInterval(1);
    glfwGetWindowPos(_window, &_winPosX, &_winPosY);
    glfwGetWindowSize(_window, &_winWidth, &_winHeight);
    // load app icon
    GLFWimage icon;
    icon.width = APPICON_W;
    icon.height = APPICON_H;
    icon.pixels = const_cast<unsigned char*>(APPICON_DATA.data());
    glfwSetWindowIcon(_window, 1, &icon);
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
    // register global hotkey
    registerHotKey();
}

AppContext::~AppContext()
{
    unregisterHotKey();
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
                    user->_fullscreen = false;
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
                    user->_fullscreen = true;
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

void AppContext::glfw_windowfocus_callback(GLFWwindow* window, int focus)
{
    // this is a fix for a bug when recording on fullscreen mode
    glfwSetWindowAttrib(window, GLFW_FLOATING, GLFW_TRUE);
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
     uniform vec3 color;\
     void main()\
     {\
        vec2 w = vec2(dW, 1.0 - dW);\
        vec2 h = vec2(dH, 1.0 - dH);\
        if(coord.x < w.y && coord.x > w.x &&\
           coord.y < h.y && coord.y > h.x)\
           {discard;}\
        else\
            {gl_FragColor = vec4(color,1.0);}\
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
    if(_fullscreen)
    {
        // in fullscreen mode, hide window if in recording
        if(_displayUI)
        {
            // restore fullscreen
            glfwShowWindow(_window);
            auto monitor = glfwGetPrimaryMonitor();
            if(monitor)
            {
                auto mode = glfwGetVideoMode(monitor);
                glfwSetWindowMonitor(_window, monitor, 0, 0, mode->width, mode->height, 0);
            }
        }
        else
        {
            // switch to windowed mode to hide
            glfwSetWindowMonitor(_window, nullptr,
                _winPosX, _winPosY, _winWidth, _winHeight, 0);
            glfwHideWindow(_window);
        }
    }
    else
    {
        // in windowed mode, do passthrough
        if(_displayUI)
        {
            glfwSetWindowAttrib(_window, GLFW_DECORATED, GLFW_TRUE);
            glfwSetWindowAttrib(_window, GLFW_MOUSE_PASSTHROUGH, GLFW_FALSE);
        }
        else
        {
            glfwSetWindowAttrib(_window, GLFW_DECORATED, GLFW_FALSE);
            glfwSetWindowAttrib(_window, GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);
        }
    }
    glfwFocusWindow(_window);
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
        else if(!_displayUI && !_fullscreen)
        {
            glUseProgram(_borderProg);
            glUniform1f(glGetUniformLocation(_borderProg, "dW"), WIN_BORDER_PIXELS / static_cast<float>(_winWidth));
            glUniform1f(glGetUniformLocation(_borderProg, "dH"), WIN_BORDER_PIXELS / static_cast<float>(_winHeight));
            glUniform3fv(glGetUniformLocation(_borderProg, "color"), 1, _borderColor.data());
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glUseProgram(0);
        }
        // refresh frame
        glfwSwapBuffers(_window);
        // check hotkey
        hotKeyPollEvents();
        // poll events
        glfwPollEvents();
    }
}

void AppContext::registerHotKey()
{

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    RegisterHotKey(glfwGetWin32Window(_window), WINHOTKEY_ID, MOD_CONTROL | MOD_NOREPEAT, VK_F10);
#elif __linux__
    auto display = XOpenDisplay(0);
    _hotkeyDpy = (void*)display;
    auto window = DefaultRootWindow(display);
    _hotkeyNum = 10;
    const std::vector<KeySym> keys = {
        XK_F10, XK_F9, XK_F8, XK_F7, XK_F6
    };
    // begin error handler and ignore any later errors
    static bool hasX11Error = false;
    auto x11_error_handler = [](Display* dpy, XErrorEvent* e) -> int
    {
        hasX11Error = true;
        return 0;
    };
    XSetErrorHandler(x11_error_handler);
    // try keys to bind
    for(auto& key : keys)
    {
        XGrabKey(display, XKeysymToKeycode(display, key), ControlMask,
            window, False, GrabModeAsync, GrabModeAsync);
        // check any error
        XSync(display, False);
        if(hasX11Error)
        {
            display_message(NAME, "failed to bind CTRL+F" + std::to_string(_hotkeyNum), MESSAGE_WARN);
            hasX11Error = false;
        }
        else break;
        _hotkeyNum--;
    }
    // check valid key
    if(_hotkeyNum < 6)
        throw std::runtime_error("failed to register global hotkey!");
    XSelectInput(display, window, KeyPressMask);
#endif
}

void AppContext::unregisterHotKey()
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    UnregisterHotKey(glfwGetWin32Window(_window), WINHOTKEY_ID);
#elif __linux__
    auto display = reinterpret_cast<Display*>(_hotkeyDpy);
    auto window = DefaultRootWindow(display);
    const std::vector<KeySym> keys = {
        XK_F10, XK_F9, XK_F8, XK_F7, XK_F6
    };
    XUngrabKey(display, XKeysymToKeycode(display, keys[10 - _hotkeyNum]), ControlMask, window);
    XCloseDisplay(display);
#endif
}

void AppContext::hotKeyPollEvents()
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    static MSG msg = {0};
    // following will cause window freeze (lol)
    // see https://docs.microsoft.com/en-us/troubleshoot/windows/win32/application-using-message-filters-unresponsive-win10
    // if(!PeekMessage(&msg, glfwGetWin32Window(_window), WM_HOTKEY, WM_HOTKEY, PM_REMOVE)) return;

    // following works
    bool hasHotkey = false;
    while(PeekMessage(&msg, glfwGetWin32Window(_window), 0, 0, PM_REMOVE))
    {
        if(msg.message == WM_HOTKEY)
        {
            hasHotkey = true;
            break;
        }
        else DispatchMessage(&msg);
    }
    if(!hasHotkey) return;
#elif __linux__
    static XEvent e;
    auto display = reinterpret_cast<Display*>(_hotkeyDpy);
    if(!XPending(display)) return;
    XNextEvent(display, &e);
    if(e.type != KeyPress) return;
#endif
    // if pressed update state
    bool success = true;
    toggleUI();
    // start/stop recording
    if(_displayUI && _mediaHandler)
        success = _mediaHandler->StopRecord();
    else if(!_displayUI && _mediaHandler)
    {
        _mediaHandler->ConfigWindow(
            _winPosX + (_fullscreen ? 0 : WIN_BORDER_PIXELS),
            _winPosY + (_fullscreen ? 0 : WIN_BORDER_PIXELS),
            _winWidth - (_fullscreen ? 0 : (WIN_BORDER_PIXELS * 2)),
            _winHeight - (_fullscreen ? 0 : (WIN_BORDER_PIXELS * 2)));
        success = _mediaHandler->StartRecord();
    }
    if(!success) toggleUI();
}