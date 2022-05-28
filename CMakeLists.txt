cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/external/cmake/")

project(Record)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -O2 -fpermissive")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
endif()

add_definitions(-DUNICODE)
add_definitions(-D_UNICODE)
add_definitions(-DGLEW_STATIC)

find_package(OpenGL REQUIRED)

option(glew-cmake_BUILD_SHARED "Build the shared glew library" OFF)
add_subdirectory(${CMAKE_SOURCE_DIR}/external/glew-cmake)

option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)
add_subdirectory(${CMAKE_SOURCE_DIR}/external/glfw)

file(GLOB IMGUI_SRC
    ${CMAKE_SOURCE_DIR}/external/imgui/*.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/*.h
    ${CMAKE_SOURCE_DIR}/external/imgui/backends/imgui_impl_opengl3.h
    ${CMAKE_SOURCE_DIR}/external/imgui/backends/imgui_impl_opengl3.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/backends/imgui_impl_glfw.h
    ${CMAKE_SOURCE_DIR}/external/imgui/backends/imgui_impl_glfw.cpp
)
add_library(ImGui ${IMGUI_SRC})
target_include_directories(ImGui PRIVATE
    ${CMAKE_SOURCE_DIR}/external/glew-cmake/include
    ${CMAKE_SOURCE_DIR}/external/glfw/include
    ${CMAKE_SOURCE_DIR}/external/imgui
    ${CMAKE_SOURCE_DIR}/external/imgui/backends
)

# find_package(PkgConfig REQUIRED)
# pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
#     libavdevice
#     libavfilter
#     libavformat
#     libavcodec
#     libswresample
#     libswscale
#     libavutil
# )
find_package(LibAV REQUIRED)

find_package(Threads REQUIRED)

file(GLOB SRC_FILES ${CMAKE_SOURCE_DIR}/src/*.cpp ${CMAKE_SOURCE_DIR}/src/*.hpp)
add_executable(recorder ${SRC_FILES})

set_target_properties(recorder
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin
)
target_include_directories(recorder PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external/glew-cmake/include
    ${CMAKE_SOURCE_DIR}/external/glfw/include
    ${CMAKE_SOURCE_DIR}/external/imgui
    ${CMAKE_SOURCE_DIR}/external/imgui/backends
    ${CMAKE_SOURCE_DIR}/external/termcolor/include
    ${LIBAV_INCLUDE_DIRS}
)
target_link_libraries(recorder PRIVATE
    libglew_static
    glfw
    ImGui
    OpenGL::GL
    ${LIBAV_LIBRARIES}
    Threads::Threads
)