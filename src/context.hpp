#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <functional>

#define DEFAULT_FPS         30
#define WIN_MIN_WIDTH       150
#define WIN_MIN_HEIGHT      150
#define WIN_BORDER_PIXELS   1

class AppContext
{
public:
    AppContext(const std::string& title);
    ~AppContext();

    bool CanLoop();
    void BeginFrame();
    void EndFrame(std::function<void()> customUI);
    void SetFPS(double fps);

    void UI();
    bool IsUIDisplay();

private:
    static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void glfw_windowsize_callback(GLFWwindow* window, int w, int h);
    static void glfw_windowpos_callback(GLFWwindow* window, int x, int y);

    void prepareOutline();

    int _winWidth, _winHeight;
    int _winPosX, _winPosY;
    std::string _title;
    double _timer, _spf;
    int _windowConfig[4];
    GLFWwindow* _window;
    bool _displayUI;
    GLuint _outlineVAO, _outlineProg;
};