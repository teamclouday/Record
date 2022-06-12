#pragma once
#include <termcolor/termcolor.hpp>

#include <iostream>
#include <string>

/** @file */

/// Message level INFO
#define MESSAGE_INFO 0

/// Message level WARN
#define MESSAGE_WARN 1

/// Message level ERROR
#define MESSAGE_ERROR 2

/**
 * @brief Print arbitrary number of arguments to console
 *
 * @tparam T Any type supported by ostream
 * @param args Arguments
 */
template <typename... T> inline void println(T &&...args)
{
    (std::cout << ... << args) << std::endl;
}

/**
 * @brief Display message with level of severity
 *
 * @param title Message title
 * @param message Message body
 * @param severity Level of severity
 */
inline void display_message(const std::string &title, const std::string &message, int severity)
{
    switch (severity)
    {
    case MESSAGE_WARN:
        std::cout << "W[" << termcolor::yellow << title << termcolor::reset << "]: " << message << std::endl;
        break;
    case MESSAGE_ERROR:
        std::cout << "E[" << termcolor::red << termcolor::blink << title << termcolor::reset << "]: " << message
                  << std::endl;
        break;
    case MESSAGE_INFO:
    default:
        std::cout << "I[" << termcolor::green << title << termcolor::reset << "]: " << message << std::endl;
        break;
    }
}
