# Design of Project

Not a formal one, just want to record some of the ideas that I would like to implement

## UI Design

Determine what to should be added and controlled on UI

### Text Info

* Window size and position  
* Window frame rate (?)  
* Control commands  
* Record output file path  


### Control

* Windows size and position (by dragging the window border)  
* UI Window Transparency  
* Border color when recording  
* Border size control  
* Record frame rate  
* Record output path and format (selected via dialogs)  
* Record audio inputs (desktop media / mic / both)  
* Record bitrate  
* ~~Delay before recording starts~~  
* Control number of frames to skip before recording  

## Other

* This program should be designed as console-based (instead of window-based) app. Otherwise I have to change `main` function to support Windows  
* So any error message will be displayed in console  
* Consider create two video/audio classes to handle two streams separately  

## Feature Thoughts  

List what features can be added for future development  

* Audio support  
  This is very necessary and has top priority  
  Also consider mixing microphone inputs  
* Add more video format supports  
  Some more common formats  
* Test on multi-monitor  
  Not sure if current build works on multi-monitor setup  
  But I don't have such environment to test it  
* Large recorded file warning  
  Not necessary, but can be a feature    
* MacOS Support  
  I need to have a Macbook first  
* Screen shots (images)  
  Not in the scope of this app  
* Video post-effects  
  Don't think it is in the scope  
  Can always use another offline app (e.g. ffmpeg)  
* Multi-instance  
  Would be cool to record itself recording!  
  But has this global key limitation  
  Idea: On start, try register F1-F10 (at most 10 instance)  
* Multi-access file  
  Two app writing on same video file?  
  Need to warn/prevent  
* Adjust font size  
  May be useful for users with 4k screen, but not necessary  