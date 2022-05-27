#pragma once
#include <string>
#include <iostream>

// display fatal error
inline void show_fatal_error(const std::string& message, const std::string& title)
{
#ifdef _WIN

#else // UNIX
    std::cerr << "[" << title << "]: " << message << std::endl;
#endif
}

