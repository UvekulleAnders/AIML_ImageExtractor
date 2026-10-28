# AIML_ImageExtractor

A Windows desktop application for browsing video frames and still images, built with MFC and OpenCV. Extracted image regions can be labeled and stored for machine learning.

## Features

- Open common video formats: AVI, MP4, MKV, MOV, WMV, FLV, MPEG/MPG, M4V and WebM (decoded through OpenCV's FFmpeg backend), as well as JPG/JPEG still images.
- View the current video frame or still image, scaled to fit the window. JPG/JPEG files open as a single-frame source.
- Step through the video with the keyboard:

  | Key | Action |
  |-----|--------|
  | Left / Right | Previous / next frame |
  | Home / End | First / last frame |

- A properties pane shows source codec, file name, file size and number of frames (JPG/JPEG sources show one frame).
- Select a region of the shown frame with the mouse (press the left button, drag, release). The region is cropped out as a PNG and stored in the speciesImages.db SQLite database.
- speciesImages.db is created next to the executable when the application starts. It has a species table and an images table (source file, frame number, position and size, PNG data).

- **Manual extraction:** drag a rectangle on the frame, then choose the species.
- **Automatic extraction:** *Image > Detect birds* (or `D`) outlines birds in the frame in green using a YOLOX model (OpenCV `dnn`); *Image > Auto extract detected birds* (`A`) stores all of them for one chosen species, and clicking a green box stores just that bird. *Detect on every frame* detects while browsing.
- **Export for machine learning:** *File > Export dataset...* writes one folder per species (`Swedish__Latin`) with `img<id>.png` files plus `manifest.csv` (image_id, file, species_id, swedish, latin, english, source_file, frame, x, y, w, h). The folder-per-class layout loads directly in PyTorch `ImageFolder` / Keras; split train/validation by source file to avoid leakage.

## Species classifier training

The separate dialog-based MFC trainer in `SpeciesModelTrainer/SpeciesModelTrainer.slnx` consumes this export, runs a PyTorch worker, and produces an ONNX model plus a `.labels.json` file. See `SpeciesModelTrainer/README.md` for setup and usage. Runtime recommendations in AIML_ImageExtractor are a later integration step.

## Requirements

- Windows
- Visual Studio with the **Desktop development with C++** workload and the **C++ MFC** component (the project uses platform toolset `v145`)
- OpenCV 4.14, with its `include` and library folders available on disk

- **Motion candidates:** a background learned from the previous 30 frames (colour change in Lab) finds small changed regions; each is zoomed and checked by YOLOX. Unconfirmed candidates are orange. Boxes get a 3 pixel padding.
- Bird detection needs the YOLOX model (about 36 MB, not stored in git): download https://huggingface.co/opencv/object_detection_yolox/resolve/main/object_detection_yolox_2022nov.onnx into a `models` folder in the repository root (or next to the exe). OpenCV must include the `dnn` module.
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
2. Select a video file or a JPG/JPEG still image. Still images open as a single frame; video files can be navigated with the keyboard shortcuts above.
3. Press the left mouse button on the displayed frame or image, drag to draw a rectangle and release. Choose a species when prompted. The cropped image is stored in speciesImages.db and the status bar confirms it.

## Unit tests

Build `GTest/AIML_ImageExtractor_Tests.vcxproj` (it is in the solution) and run `GTest\x64\<Configuration>\AIML_ImageExtractor_Tests.exe` with the OpenCV `bin` folder on `PATH`. The detector test is skipped if the YOLOX model is missing.

## Project layout

| File(s) | Purpose |
|---------|---------|
| `AIML_ImageExtractor.cpp/.h` | Application class and file-open dialog |
| `AIML_ImageExtractorDoc.cpp/.h` | Document: opens video and JPG/JPEG sources and provides the current frame/image |
| `AIML_ImageExtractorView.cpp/.h` | View: draws the current frame, handles keyboard navigation and the mouse selection of image regions |
| `SpeciesDatabase.cpp/.h` | SQLite wrapper for `speciesImages.db` |
| `AssignSpeciesDlg.cpp/.h` | Dialog that assigns an extracted image to a species or creates a new one |
| `WikipediaLookup.cpp/.h` | Suggests Latin and English species names from Wikipedia (WinHTTP) |
| `third_party/sqlite` | SQLite 3.54.1 amalgamation, compiled into the application |
| `GTest/` | GoogleTest project `AIML_ImageExtractor_Tests` with unit tests for the non-MFC code (vendored GoogleTest in `GTest/googletest`) |
| `MainFrm`, `ChildFrm` | Main and MDI child frame windows |
| `PropertiesWnd` | Properties pane with source information |
| `ClassView`, `FileView`, `OutputWnd`, `ViewTree` | Docking panes from the MFC project template |
| `AIMLImageExtractor.rc`, `Resource.h`, `res/` | Resources: icons, bitmaps, menus and dialogs |

## License

Released under the [MIT License](LICENSE).
