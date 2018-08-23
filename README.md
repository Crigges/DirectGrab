# DirectGrab

DirectGrab is a simple and minimalistic API to grab frames from any UVC compatible Video Device on Windows. 
It allows you to grab the frames directly from the camera without any preprocessing e.g. MJPEG, H264.

## Why DirectGrab

Other Camera APIs like the one included in OpenCV only offer limited access to different resolutions, framerates and capture formats. 
To get full control about these settings, you are basically forced to use DirectShow on Windows.  
  
If you already know the DirectShow API and you don't mind its verbosity, then there is no reason to use this library. 
But if you are not familiar with DirectShow and you just want to grab some frames from your webcam with specific settings, 
than this library is the way to go. 

## How to Use  
The API should be self explanatory, but I'll add a detailed guidance when I find some time for it.

## How to Build  
Use the given project file to build the Project in VS2017+.
