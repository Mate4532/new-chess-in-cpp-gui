# Qt-based Chess Graphical User Interface & Framework

![C++](https://img.shields.io/badge/Language-C++-blue.svg)
![Framework](https://img.shields.io/badge/Framework-Qt-green.svg)
![Status](https://img.shields.io/badge/Status-Work_in_Progress-orange.svg)

A modular and responsive desktop application built with the **Qt framework**. The project is designed as a flexible framework where core components are decoupled through well-defined interfaces to ensure maximum extensibility.

> **Core Engine:**
> The frontend is separated from the backend logic. You can find the high-performance, 40M NPS C++ engine repository here: **[[engine](https://github.com/Mate4532/new-chess-in-cpp)]**

## Architecture & Design Philosophy
The primary goal of this GUI is **strict modularity** and the application of clean software design principles (SOLID).

### Key Architectural Features:
* **Interface-Driven Component Swapping:** Currently, the engine communicates with the GUI through a dedicated C++ interface. This allows any engine implementation to be "plugged in" as long as it satisfies the interface requirements.
* **Planned Pluggable Board Logic:** A key part of the roadmap is making the **Board representation** interchangeable via interfaces. This will ensure that the underlying board logic can be replaced without affecting the GUI or the engine.
* **Transition to UCI:** While the current stage uses internal interfaces for simplicity, the long-term goal is to replace the custom interface layer with the **UCI (Universal Chess Interface) protocol**.

## Configurable Engine & Game Settings
The framework implements a robust settings management system (`RobotSettings`, `BoardSettings`, `TimeSettings`), allowing for granular control over the environment:

* **Engine Configuration:** Multi-threading support (configurable `numThreads`), search time control, and multiple matchup modes (Human vs Human, Human vs. Bot, Bot vs. Bot).
* **Board & Visuals:** Support for any starting position via FEN strings, toggleable legal move highlighting, and board flipping.
* **Time Management:** Support for both Unlimited Thinking Time and Tournament modes with Fisher-style increments.

## Development Status & To-Do
**This project is actively maintained. The core logic and performance targets are achieved, but the codebase is currently undergoing a refactoring phase to improve readability and maintainability.**

* **Board Interface Implementation:** Developing the abstraction layer for interchangeable board representations.
* **Native UCI Support:** Transitioning to full external process communication via the UCI protocol.

## Installation & Running

### Requirements
* **Windows 10/11, 64-bit**
* **A CPU with AVX2.** The build uses `-march=native`, so the executable is tuned for the machine that compiled it and may not start on older CPUs.
* **Qt 6 with the MinGW 64-bit kit** (tested with Qt 6.10.1 and MinGW 13.1.0). In the [Qt Online Installer](https://www.qt.io/download-qt-installer) select:
  * `Qt → Qt 6.10.1 → MinGW 13.1.0 64-bit`
  * `Qt → Developer and Designer Tools → MinGW 13.1.0 64-bit`, `CMake`, `Ninja`
* **MSVC is not supported.** The HalfKAv2 code uses GCC builtins (`__builtin_popcountll`), so use the MinGW kit.

In the paths below, `<Qt>` is your Qt install folder, e.g. `C:/Qt`.

### Clone
```
git clone https://github.com/Mate4532/new-chess-in-cpp-gui.git
```
The NNUE networks are included in the repository (`nnues/halfkp/` and `nnues/halfkav2_hm/`), so nothing needs to be downloaded separately.

### Qt Creator
1. **File → Open File or Project…** and select `CMakeLists.txt`.
2. On the *Configure Project* page, enable the kit **Desktop Qt 6.10.1 MinGW 64-bit** and click **Configure Project**.
3. Build with **Ctrl+B** and run with **Ctrl+R**.

Qt Creator adds the Qt DLLs to `PATH` when it runs the app, so no extra steps are needed.

### CLion
1. **File → Open…** and select the repository folder.
2. **Settings → Build, Execution, Deployment → Toolchains**: add a **MinGW** toolchain:
   * *Toolset*: `<Qt>/Tools/mingw1310_64`
   * *CMake*: the bundled one, or `<Qt>/Tools/CMake_64/bin/cmake.exe`
   * Move it to the top of the list so it becomes the default.
3. **Settings → Build, Execution, Deployment → CMake**: for each profile (Debug/Release) set:
   * *Toolchain*: the MinGW toolchain from step 2
   * *Generator*: `Ninja`
   * *CMake options*: `-DCMAKE_PREFIX_PATH=<Qt>/6.10.1/mingw_64`
4. **Run → Edit Configurations… → new-chess-in-cpp-gui → Environment variables**: add
   ```
   PATH=<Qt>\6.10.1\mingw_64\bin;<Qt>\Tools\mingw1310_64\bin;$PATH$
   ```
   CLion does not add Qt to `PATH` itself. Without this the app exits immediately with missing `Qt6Core.dll` / `Qt6Widgets.dll` errors.
5. Reload the CMake project, then build and run.

### Running outside the IDE
To start the `.exe` directly, copy the Qt DLLs next to it once:
```
<Qt>/6.10.1/mingw_64/bin/windeployqt.exe path/to/new-chess-in-cpp-gui.exe
```

### Build notes
* **NNUE files are copied automatically.** After every build, CMake copies the selected nets into `<build dir>/nnues/`, so you never place them by hand. The nets are CMake cache variables, so you can switch them in Qt Creator's *Projects → CMake* page or CLion's *CMake options* (e.g. `-DNNUE_HALFKAV2_FILE=`):
  * `NNUE_FILE`: the HalfKP net, default `halfkp/nn-62ef826d1a6d.nnue`
  * `NNUE_HALFKAV2_FILE`: the HalfKAv2_hm net used by the improved bot, default `halfkav2_hm/nn-dabb1ed23026.nnue`. Set it to an empty value to use HalfKP only.
* **Every build is a Release build.** `CMakeLists.txt` forces `CMAKE_BUILD_TYPE Release` (`-O3`, LTO), even in the Debug profile. To step through code in a debugger, remove that line.
* **Asset lookup.** At runtime, `assets/` (e.g. `openings.txt`) is found by searching up to 5 parent folders from the working directory. The default IDE build folders (`build/...`, `cmake-build-*`) work without changes.
* **Generated folders.** Bot-vs-bot games write results to `botvsbotresults/` and source snapshots of each bot version to `generatedbotversions/`. Both are recreated automatically.