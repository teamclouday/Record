# Record

A cross-platform screen capture application that is able to record a user-defined area on screen.  
On Linux there is an awesome screen capture application ([Peek](https://github.com/phw/peek)) but on Windows I cannot find anything. That's why this project is created.  
This project aims to have similar features as Peek.

------

## Platform  

Current supported platforms are:  
* Linux (X11 backend)  

## Formats

Current supported output video formats are:  
* mp4  
* mkv

## Feature Plans

- [ ] Global Hotkey for Window Refocus  
- [ ] Windows Support  
- [ ] Audio Capture  
- [ ] Microphone Audio  
- [ ] Fully Support Fullscreen Capture  
- [ ] Support GIF  
- [ ] Support Webm  

## Dependencies

* [ImGui](https://github.com/ocornut/imgui)  
* [GLFW 3.4](https://github.com/glfw/glfw)  
* [GLEW](http://glew.sourceforge.net/)  
* [FFmpeg 5.0 Libav](https://github.com/FFmpeg/FFmpeg)  
* [termcolor](https://github.com/ikalnytskyi/termcolor)  

## Compilation

First pull the repo:
```bash
git submodule update --init --recursive
```

<details>
<summary>Steps for Linux</summary>

On linux, make sure `ffmpeg` (version 5.0) and related `libav` libraries are installed. If you are not sure:
```bash
ldconfig -p | grep libav
ldconfig -p | grep libsw
```
and look for the following libraries:
* `libavdevice`  
* `libavfilter`  
* `libavformat`  
* `libavcodec`  
* `libswresample`  
* `libswscale`  
* `libavutil`  

If you are not using X11, run following:
```bash
export GDK_BACKEND=x11
```
to force x11 backend.

</details>

Then init build files:
```bash
mkdir build
cd build
cmake ..
```

Build (Linux):
```bash
make -j4
```

Build (Windows):
```powershell
cmake --build . --config Release
```

------

## Releases

work in progress...