# MotionTracking MK-II Plus for AviUtl2

AviUtl ExEdit2 object tracking (rubbish) plugin based on LKT/optical flow.

## System Requirement

- CPU with AVX2 support
- Windows 10 or later
- DirectX11.3 support
- AviUtl ExEdit2 version 2.0beta48 or later
- Tested with AviUtl ExEdit2 version 2.1.0

## Installation

Dump the .aux2 file into  
`your favorite folder where AviUtl loads aux2 file`  
Create the `MotionTracking_model` folder in the same directory as `MotionTrackingMKIIPlusforAviUtl2.aux2`.

The menu name should be "MotionTracking MK-II Plus for AviUtl2"

Additional work is required if DaSiamRPN, Nano or Vit are to be used.

### DaSiamRPN

1. Download the following file from the URL listed as a comment out in the source code at [this URL](https://github.com/opencv/opencv/blob/4.x/samples/dnn/dasiamrpn_tracker.cpp)

- dasiamrpn_model.onnx
- dasiamrpn_kernel_r1.onnx
- dasiamrpn_kernel_cls1.onnx

2. Dump each files into `MotionTracking_model folder`

### Nano

1. Download the following file from [this URL](https://github.com/HonglinChu/SiamTrackers/tree/18b7791360acb3f6d276d47376a6f1ed516f1628/NanoTrack/models/nanotrackv2)

- nanotrack_backbone_sim.onnx
- nanotrack_head_sim.onnx

2. Dump each files into `MotionTracking_model folder`

### Vit

1. Download the following file from [this URL](https://github.com/opencv/opencv_extra/blob/4.x/testdata/dnn/onnx/models/vitTracker.onnx)

- vitTracker.onnx

2. Dump each files into `MotionTracking_model folder`

## Helper Plugins

> [!Important]
> At this stage, Helper Plugins are not supported. I plan to add support in a future update.

The single AUX2 actually contains two more helper plugins:

1. Pre-track: HSV Cvt
2. Pre-track: BGSubtraction

HSVCvt convert the RGB image into HSV, then display it as if RGB. It can also display only one of the HSV channels.

BGSubtraction aims to isolate the moving object from the background. It can output the isolated RGB image, or output a grey-scale mask. Beware that a large Range value may cause out-of-memory problem, or enable Large-Address-Aware to get around.

## Help

### MotionTracking MK-II Plus

#### Steps

0. Mark a section to track
1. Click 1st button, Drag a box on the object to be tracked(in popup Window). Close the popup Window by clicking the x button or pressing F3.
2. Click Analyze, wait for completion. To cancel the analysis, click the X button.
3. Activate the View Result and check. If result is good, check Invert Position if necessary, click Insert Object or save Object file. Otherwise, click Clear Result and go back to step 0 or 1.

#### Export Object File

Auto correct for single sandwiched error result.  
Support CJK filename

#### Options

#### Dropdown options

##### Method

Specifies the algorithm to be used in the analysis.

1. Multi Instance Learning
2. KCF
3. CSRT
4. DaSiamRPN
5. Nano
6. Vit

##### Hue

Specifies the hue of the rectangle displayed in Object Selection and View Result.

##### Wnd Scale

This is the window scale of Select Object / View Result window. You can select a value from 0.00 to 1.00. If you select track bar as far right(--), it becomes disabled.
The trackbar in View Result can become extremely small, because this plugin forcibly overrides the window size from outside. Please be careful when using this in View Result.

##### Insert Object Options

- As Sub-filter/部分フィルタ？ : Output as a sub filter.
- Invert Position : Reverse the position of the tracking result.
- Ignore Aspect Ratio : Ignore the aspect ratio and output in scale.

### Pre-track:BGSubtraction

> [!Important]
> At this stage, Pre-track:BGSubtraction is not supported. I plan to add support in a future update.

#### Common Parameters

- Range : Use <Range> no. of frames before and after current frame for analysis.[30]
- Shadow : 1= Extract shadow [0]

#### MOG2-Only

- NMix : Number of Gaussian mixtures [5]
- BG% : Background ratio [70%]

#### KNN-Only

- d2T : Threshold on the squared distance between the pixel and the sample to decide whether a pixel is close to that sample.

## Building From Source

Please read `.github/workflows/build.yml`.

Below are the steps to build with MSVC on Windows (both Release and Debug).

### Prerequisites

- Windows 10/11
- [Visual Studio 2022](https://visualstudio.microsoft.com/) with the "Desktop development with C++" workload (MSVC v143 toolset, Windows 10/11 SDK)
- [Git](https://git-scm.com/) (to fetch submodules)
- [Python](https://www.python.org/) 3.13 or later
- [Poetry](https://python-poetry.org/) (used to install Conan into a managed virtual environment)
- [Chocolatey](https://chocolatey.org/) (used to install make / CMake)
- make
- [CMake](https://cmake.org/) 3.20 or later

You can install Python from the [python.org](https://www.python.org/) installer or via winget:

```powershell
winget install --id Python.Python.3.13 -e
```

The winget package sets up the `py` launcher and `PATH` automatically. After installing, **open a new shell** and confirm `py --version` reports 3.13 or later.

> Note: This is normally unnecessary. But if a Microsoft Store stub (`WindowsApps\python.exe`) from a previous Python setup takes precedence and the `python` command opens the Store instead, disable `python.exe` / `python3.exe` under Settings > Apps > App execution aliases.

### 1. Clone the repository

Make sure to fetch the `src/aviutl2_sdk` submodule as well.

```bash
git clone --recursive https://github.com/nullruptr/MotionTracking_MK-II_Plus_for_AviUtl2.git
cd MotionTracking_MK-II_Plus_for_AviUtl2
```

If you already cloned without submodules:

```bash
git submodule update --init --recursive
```

### 2. Install Poetry

Use the official installer. (See the [Poetry docs](https://python-poetry.org/docs/#installing-with-the-official-installer).)

```powershell
(Invoke-WebRequest -Uri https://install.python-poetry.org -UseBasicParsing).Content | py -
```

Confirm `poetry --version` works afterwards. If the command is not found, add Poetry's install location (`%APPDATA%\pypoetry\venv\Scripts`) to your user `PATH`:

```powershell
$p = "$env:APPDATA\pypoetry\venv\Scripts"
[Environment]::SetEnvironmentVariable("Path",[Environment]::GetEnvironmentVariable("Path","User")+";$p","User")
```

The `PATH` change takes effect in newly opened shells. Restart your shell and re-check `poetry --version`.

### 3. Set up Conan

From the repository root, install the Poetry-managed virtual environment, which includes Conan:

```bash
poetry install
```

This installs `conan` (Conan 2.x), declared in `pyproject.toml`, into the virtual environment. Run subsequent Conan commands as `poetry run conan ...`.

If this is your first time using Conan on this machine (i.e. `~/.conan2/profiles/default` doesn't exist yet), create the default build-context profile once:

```bash
poetry run conan profile detect --force
```

The host-context profiles actually used for the build are already provided in the repository, so you don't need to create them yourself:

- Release: [`profiles/msvc-release`](profiles/msvc-release)
- Debug: [`profiles/msvc-debug`](profiles/msvc-debug)

### 4. Build with MSVC

The first build also builds the dependency (OpenCV) from source, so it can take 30+ minutes. It's recommended to run the build from an "x64 Native Tools Command Prompt for VS 2022" or "Developer PowerShell for VS 2022".

#### Setting up make / CMake

Install `make` and CMake with [Chocolatey](https://chocolatey.org/) (same setup as CI).

If Chocolatey is not installed yet, install it first by following the [official instructions](https://chocolatey.org/install) in an **elevated PowerShell**.

Then, in an **elevated PowerShell** (right-click "PowerShell" in the Start menu and choose "Run as administrator"), run:

```powershell
choco install make -y
choco install cmake -y
```

After installation, **open a new shell** and confirm `make --version` and `cmake --version` (3.20 or later) work (Chocolatey sets up `PATH` automatically).

> Note: The "Desktop development with C++" workload of Visual Studio 2022 ships CMake, and running from a "Developer PowerShell for VS 2022" puts it on `PATH`, so `choco install cmake` is not required in that case (installing it anyway is harmless).

Once make is set up, run the following:

```bash
cd src

# Release build
make compile

# Debug build
make compile-debug
```

If you use clangd, run `make compdb` separately after building to generate `compile_commands.json`.

### 5. Build output

On success, the `.aux2` file is generated at:

- Release: `src/build/build/Release/MotionTrackingMKIIPlusforAviUtl2.aux2`
- Debug: `src/build-debug/build/Debug/MotionTrackingMKIIPlusforAviUtl2.aux2`

Place the generated `.aux2` file, along with the `MotionTracking_model` folder, into the directory where AviUtl ExEdit2 loads general plugins (see the "Installation" section above).

### 6. Debugging

Debug as follows:

1. Copy the Debug build's `MotionTrackingMKIIPlusforAviUtl2.aux2` and `MotionTrackingMKIIPlusforAviUtl2.pdb` into the plugin loading directory
2. Open `src/build-debug/build/MotionTrackingMKIIPlusforAviUtl2.sln` in Visual Studio
3. In Solution Explorer, right-click `MotionTrackingMKIIPlusforAviUtl2`
4. From the context menu, click `Set as Startup Project`
5. Confirm that `MotionTrackingMKIIPlusforAviUtl2` is now shown in bold
6. Right-click `MotionTrackingMKIIPlusforAviUtl2` again -> Properties, to open the `MotionTrackingMKIIPlusforAviUtl2 Property Pages` dialog
7. In the tree on the left, open Configuration Properties -> Debugging
8. Set Command to the full path of `aviutl2.exe` (with the default AviUtl2 install location, `C:\Program Files\AviUtl2\aviutl2.exe`)
9. Set Working Directory to the folder containing `aviutl2.exe` (with the default AviUtl2 install location, `C:\Program Files\AviUtl2`)
10. Apply the changes on the Property Pages dialog and close it
11. Press F5 to start debugging

## Bug Report

- [GitHub](https://github.com/nullruptr/MotionTracking_MK-II_Plus_for_AviUtl2)
