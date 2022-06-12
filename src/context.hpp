#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "media.hpp"

#include <array>
#include <functional>
#include <memory>
#include <string>

/** @file */

/// Window minimum width
#define WIN_MIN_WIDTH 150

/// Window minimum height
#define WIN_MIN_HEIGHT 150

/// Window border number of pixels
#define WIN_BORDER_PIXELS 1

/// Window UI font size
#define WIN_UI_FONT_SIZE 25

/**
 * @brief Application Context
 *
 * This class manages main window and user interaction events.
 */
class AppContext
{
  public:
    AppContext(const std::string &title);
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
     * Attach a MediaHandler instance to update record events.
     *
     * @param handler A pointer to MediaHandler instance
     */
    void AttachHandler(std::shared_ptr<MediaHandler> handler);

    /**
     * @brief UI Calls
     *
     * Is meant to be called in customUI function.
     */
    void UI();

    /// Defined name for message title
    const std::string NAME = "AppContext";

  private:
    /// GLFW key callback
    static void glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

    /// GLFW window size callback
    static void glfw_windowsize_callback(GLFWwindow *window, int w, int h);

    /// GLFW window position callback
    static void glfw_windowpos_callback(GLFWwindow *window, int x, int y);

    /// GLFW window focus state callback
    static void glfw_windowfocus_callback(GLFWwindow *window, int focus);

    /// Prepare shader program to draw border
    void prepareBorder();

    /// toggle UI and related window states
    void toggleUI();

    /// register global hotkey to refocus window
    void registerHotKey();

    /// unregister global hotkey
    void unregisterHotKey();

    /// handle hot key events
    void hotKeyPollEvents();

    int _winWidth, _winHeight;
    int _winPosX, _winPosY;
    int _monWidth, _monHeight;
    float _alpha;
    std::string _title;
    int _windowConfig[4];
    GLFWwindow *_window;
    bool _displayUI, _fullscreen;
    GLuint _borderProg;
    int _borderNumPixels;
    std::array<float, 3> _borderColor; // vec3
    void *_hotkeyDpy;                  // X11 display for hotkey events
    int _hotkeyNum;

    std::shared_ptr<MediaHandler> _mediaHandler;
};
