#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "video.hpp"
#include <string>
#include <memory>
#include <functional>

/** @file */

/// Window minimum width
#define WIN_MIN_WIDTH       150

/// Window minimum height
#define WIN_MIN_HEIGHT      150

/// Window border number of pixels
#define WIN_BORDER_PIXELS   1

/**
 * @brief Application Context
 * 
 * This class manages main window and user interaction events.
 */
class AppContext
{
public:
    AppContext(const std::string& title);
    ~AppContext();

    /**
     * @brief Application Internal Loop
     * 
     * Executes a loop for internal rendering and poll events.
     * 
     * @param customUI A custom UI function with ImGui
     */
    void AppLoop(std::function<void()> customUI);

    /**
     * @brief Attach Video Handler
     * 
     * Attach a VideoHandler instance to update record events.
     * 
     * @param handler A pointer to VideoHandler instance
     */
    void AttachHandler(std::shared_ptr<VideoHandler> handler);

    /**
     * @brief UI Calls
     * 
     * Is meant to be called in customUI function.
     * 
     */
    void UI();

    /// Defined name for message title
    const std::string NAME = "AppContext";

private:
    /// GLFW key callback
    static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

    /// GLFW window size callback
    static void glfw_windowsize_callback(GLFWwindow* window, int w, int h);

    /// GLFW window position callback
    static void glfw_windowpos_callback(GLFWwindow* window, int x, int y);

    /// Prepare shader program to draw border
    void prepareBorder();

    int _winWidth, _winHeight;
    int _winPosX, _winPosY;
    std::string _title;
    int _windowConfig[4];
    GLFWwindow* _window;
    bool _displayUI;
    GLuint _borderProg;

    std::shared_ptr<VideoHandler> _videoHandler;
};