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
- Select a region of the shown frame with the mouse (press the left button, drag, release). The region is cropped out as a PNG and stored in the speciesImages.db SQLite database.
- speciesImages.db is created next to the executable when the application starts. It has a species table and an images table (source file, frame number, position and size, PNG data).

## Requirements

- Windows
- Visual Studio with the **Desktop development with C++** workload and the **C++ MFC** component (the project uses platform toolset `v145`)
- OpenCV 4.14, with its `include` and library folders available on disk

## Building

1. Build OpenCV 4.14 (modules linked: `core`, `imgproc`, `imgcodecs`, `highgui`, `video`, `videoio`) and install it into one folder per configuration:

   ```
   C:\opencv\opencv\out\install\
   ├─ x64-Debug\
   │  ├─ include\
   │  └─ x64\vc18\lib\
   └─ x64-Release\
      ├─ include\
      └─ x64\vc18\lib\
   ```

2. Open `AIML_ImageExtractor.slnx` in Visual Studio, or build from the command line:

   ```powershell
   msbuild AIML_ImageExtractor.vcxproj /p:Configuration=Release /p:Platform=x64
   ```

OpenCV is configured through MSBuild properties in `AIML_ImageExtractor.vcxproj`:

| Property | Meaning | Default |
|----------|---------|---------|
| `OpenCV_DIR` | Base folder that contains the `x64-Debug` and `x64-Release` installs | `C:\opencv\opencv\out\install` |
| `OpenCVBaseDir` | Install folder for the active configuration | `$(OpenCV_DIR)\x64-Debug` or `$(OpenCV_DIR)\x64-Release` |
| `OpenCVIncludeDir` | Include folder | `$(OpenCVBaseDir)\include` |
| `OpenCVLibraryDir` | Library folder | `$(OpenCVBaseDir)\x64\vc18\lib` |
| `OpenCVVersion` | Library version suffix | `4140` |
| `OpenCV_VC` | Visual C++ folder name in the OpenCV layout | `vc18` |

Set `OpenCV_DIR` to use another location, for example `/p:OpenCV_DIR=D:\libs\opencv`. Debug builds link the `*d.lib` libraries and Release builds link the release libraries.
## Usage

1. Start the application and choose **File > Open**.
2. Select a video file.
3. Use the keyboard shortcuts above to move between frames.
4. Press the left mouse button on the frame, drag to draw a rectangle and release. The cropped image is stored in speciesImages.db and the status bar confirms it.

## Project layout

| File(s) | Purpose |
|---------|---------|
| `AIML_ImageExtractor.cpp/.h` | Application class and file-open dialog |
| `AIML_ImageExtractorDoc.cpp/.h` | Document: opens the video and reads frames with OpenCV |
| `AIML_ImageExtractorView.cpp/.h` | View: draws the current frame, handles keyboard navigation and the mouse selection of image regions |
| `SpeciesDatabase.cpp/.h` | SQLite wrapper for `speciesImages.db` |
| `third_party/sqlite` | SQLite 3.54.1 amalgamation, compiled into the application |
| `MainFrm`, `ChildFrm` | Main and MDI child frame windows |
| `PropertiesWnd` | Properties pane with video information |
| `ClassView`, `FileView`, `OutputWnd`, `ViewTree` | Docking panes from the MFC project template |
| `AIMLImageExtractor.rc`, `Resource.h`, `res/` | Resources: icons, bitmaps, menus and dialogs |

## License

Released under the [MIT License](LICENSE).
