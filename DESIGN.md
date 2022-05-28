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
* Record frame rate  
* Record output path and format (selected via dialogs)  
* Record audio inputs (desktop media / mic / both)  
* Record bitrate  

## Recorder Logic

1. Init stage, initialize necessary buffers, register AVdevices.  
2. 

## Other

* This program should be designed as console-based (instead of window-based) app. Otherwise I have to change `main` function to support Windows.  
* So any error message will be displayed in console.  

## Future Plan  

List what features can be added for future development  

