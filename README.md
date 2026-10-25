# AIML_ImageExtractor

A Windows desktop application for browsing a video frame by frame, built with MFC and OpenCV. The goal is to extract individual images from video files.

## Features

- Open common video formats: AVI, MP4, MKV, MOV, WMV, FLV, MPEG/MPG, M4V and WebM (decoded through OpenCV's FFmpeg backend).
- View the current frame, scaled to fit the window.
- Step through the video with the keyboard:

  | Key | Action |
  |-----|--------|
  | Left / Right | Previous / next frame |
  | Home / End | First / last frame |

- A properties pane shows the video's codec, file name, file size and number of frames.

## Requirements

- Windows
- Visual Studio with the **Desktop development with C++** workload and the **C++ MFC** component (the project uses platform toolset `v145`)
- OpenCV, with its `include` and library folders available on disk

## Building

1. Install or build OpenCV (modules used: `core`, `videoio`, `imgproc`).
2. Open `AIML_ImageExtractor.slnx` in Visual Studio, or build from the command line:

   ```powershell
   msbuild AIML_ImageExtractor.slnx /p:Configuration=Release /p:Platform=x64 /p:OpenCV_DIR=C:\opencv\build
   ```

OpenCV is configured through MSBuild properties in `AIML_ImageExtractor.vcxproj`:

| Property | Meaning | Default |
|----------|---------|---------|
| `OpenCV_DIR` | OpenCV build directory (the one containing `include`) | none, must be set |
| `OpenCVVersion` | Library version suffix | `2413` |
| `OpenCV_VC` | Visual C++ folder name in the OpenCV layout | `vc14` |
| `OpenCVLibraryDir` | Library folder | `$(OpenCV_DIR)\<x86\|x64>\<OpenCV_VC>\lib` |

Override `OpenCVVersion`, `OpenCV_VC` or `OpenCVLibraryDir` when your OpenCV installation differs. Debug builds link the debug libraries and Release builds link the release libraries.

> **Note:** The source uses the `cv::CAP_*` constants, which exist in OpenCV 3 and later, and the x64 Debug configuration also contains a hard-coded `#pragma comment(lib, ...)` path under `C:\opencv\opencv\out\install\x64-Debug\x64\vc18\lib`. Adjust the defaults above to match the OpenCV version you actually build against.

## Usage

1. Start the application and choose **File > Open**.
2. Select a video file.
3. Use the keyboard shortcuts above to move between frames.

## Project layout

| File(s) | Purpose |
|---------|---------|
| `AIML_ImageExtractor.cpp/.h` | Application class and file-open dialog |
| `AIML_ImageExtractorDoc.cpp/.h` | Document: opens the video and reads frames with OpenCV |
| `AIML_ImageExtractorView.cpp/.h` | View: draws the current frame and handles keyboard navigation |
| `MainFrm`, `ChildFrm` | Main and MDI child frame windows |
| `PropertiesWnd` | Properties pane with video information |
| `ClassView`, `FileView`, `OutputWnd`, `ViewTree` | Docking panes from the MFC project template |
| `AIMLImageExtractor.rc`, `Resource.h`, `res/` | Resources: icons, bitmaps, menus and dialogs |

## License

Released under the [MIT License](LICENSE).
