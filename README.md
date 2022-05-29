# Record

A cross-platform screen capture application that is able to record a user-defined area on screen.  
On Linux there is an awesome screen capture application ([Peek](https://github.com/phw/peek)) but on Windows I cannot find anything. That's why this project is created.  
This project aims to have similar features as Peek.

------

## Controls

* `ESC`: exit app  
* `F11`: toggle fullscreen  
* `CTRL+F10` (global hotkey): start/stop recording  

Because of global hotkey registration, this application can only launch one instance (at least on linux X11) at a time.

## Platform  

Current supported platforms are:  
* Linux (X11 backend)  
* Windows  

Note that on Windows it is a static build, while on Linux it is shared.

## Formats

Current supported output video formats are:  
* mp4  
* mkv

## Feature Plans

- [x] Global Hotkey for Window Refocus  
- [x] Windows Support  
- [ ] Audio Capture  
- [ ] Microphone Audio  
- [x] Fully Support Fullscreen Capture  
- [ ] Support GIF  
- [ ] Support Webm  

## Dependencies

* [ImGui](https://github.com/ocornut/imgui)  
* [GLFW 3.4](https://github.com/glfw/glfw)  
* [GLEW](http://glew.sourceforge.net/)  
* [FFmpeg 5.0 Libav](https://github.com/FFmpeg/FFmpeg)  
* [termcolor](https://github.com/ikalnytskyi/termcolor)  

<details>
<summary>Windows Additional Libs</summary>

The following libs are required for Windows static build, but should all exist in a standard Windows environment:  
* comdlg32.lib  
* mfplat.lib  
* mfuuid.lib  
* strmiids.lib  
* secur32.lib  
* shlwapi.lib  
* vfw32.lib  
* ws2_32.lib  
* bcrypt.lib  

</details>

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