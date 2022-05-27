#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <functional>

#define DEFAULT_FPS     30

class AppContext
{
public:
    AppContext(const std::string& title);
    ~AppContext();

    bool canLoop();
    void beginFrame();
    void endFrame(std::function<void()> customUI);
    void setFPS(double fps);

    void UI();

private:
    static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

    int _winWidth, _winHeight;
    std::string _title;
    double _timer, _spf;
    int _windowConfig[4];
    GLFWwindow* _window;
    bool _displayUI;
};