# AIML_ImageExtractor

OpenCV 2.4 is configured through MSBuild properties. Set `OpenCV_DIR` to the OpenCV build directory (the directory containing `include`), for example with `/p:OpenCV_DIR=C:\opencv\build`. The default library suffix is `2413` and the default library folder is `$(OpenCV_DIR)\x86\vc14\lib` for Win32 or `$(OpenCV_DIR)\x64\vc14\lib` for x64; override `OpenCVVersion`, `OpenCV_VC`, or `OpenCVLibraryDir` when your OpenCV 2.4 installation differs. Debug builds link the debug libraries; Release builds link the release libraries.
