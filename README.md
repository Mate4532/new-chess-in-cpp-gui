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
1. Clone the repository and build the project using **Qt Creator** or **CMake**.
2. **Important:** For the evaluation to function correctly, the **NNUE evaluation file** must be placed in the same directory as the compiled executable (`.exe`).
3. Ensure the engine implementation is correctly linked or satisfies the required C++ interface.