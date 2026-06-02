# NetSpeedMeter

[![Download Latest Release](https://img.shields.io/github/v/release/sayedalve/netspeedmeter?label=Download%20Latest%20Release\&style=for-the-badge\&color=success)](https://github.com/sayedalve/netspeedmeter/releases/latest)

A lightweight, high performance network speed monitor that integrates seamlessly into the Windows taskbar. Built with **C++17** and **Qt 6**, NetSpeedMeter uses native Windows APIs to provide accurate, real time download and upload speed metrics directly within your workspace without unnecessary clutter.

---

## 📥 Download & Install

**You do not need to compile this application to use it.**

### Quick Start

1. Download the latest release from:
   https://github.com/sayedalve/netspeedmeter/releases/latest

2. Extract the downloaded `.zip` file.

3. Run:

```text
NetSpeedMeter.exe
```

No installation is required.

### System Requirements

* Windows 10 (64 bit)
* Windows 11 (64 bit)

---

## 🚀 Features

### Native Taskbar Integration

* Frameless and transparent interface designed to blend naturally with the Windows taskbar.
* Advanced Z order locking mechanism prevents the overlay from slipping behind the taskbar during interaction.
* Clean and distraction free design.

### Intelligent Visibility Management

* Automatically hides when fullscreen applications are detected.
* Supports games, videos, presentations, and media playback.
* Detects Windows Start Menu and Search panels to prevent UI conflicts.

### Windows 11 Input Handling

* Custom low level mouse event interception.
* Prevents Windows 11 shell input interception issues.
* Responsive dark themed context menu.

### Accurate Network Monitoring

* Uses native Windows **GetIfEntry2** APIs.
* Reads 64 bit network interface byte counters directly from the operating system.
* Provides accurate upload and download speed measurements.

### Single Instance Protection

* Prevents multiple copies of the application from running simultaneously.
* Implemented using **QSharedMemory**.

### Advanced Customization

* Font family selection.
* Adjustable font sizes.
* Normal and bold font weights.
* UI scaling controls.
* Multiple speed unit modes:

  * Auto
  * KB/s
  * MB/s
  * Bits/s
* Adjustable opacity settings.

---

## 🛠️ Architecture & Technical Design

### Dedicated QThread Monitoring Engine

Traditional `QTimer` implementations execute on the main UI thread, which can introduce lag during intensive processing.

NetSpeedMeter moves network calculations to a dedicated background thread using **QThread**, ensuring:

* Smooth UI responsiveness.
* Reduced frame drops.
* Improved monitoring stability.

### High Precision Speed Calculation

System timer intervals can fluctuate under varying CPU workloads.

To maintain accuracy, NetSpeedMeter uses:

* `QElapsedTimer` for high resolution timing.
* Exact elapsed time measurements.
* Delta over elapsed time calculations for bandwidth estimation.

This approach produces highly accurate real time speed reporting regardless of system load.

### Safe Configuration Management

Rapid UI updates can introduce configuration race conditions.

NetSpeedMeter addresses this through:

* Explicit configuration saves during application shutdown or settings confirmation.
* Startup configuration locks during UI initialization.
* Protection against unintended configuration overwrites.

---

## 💻 Building from Source

### Prerequisites

| Requirement      | Version                       |
| ---------------- | ----------------------------- |
| Operating System | Windows 10 / 11 (64 bit)      |
| Compiler         | MSVC 2019 / 2022 or MinGW 12+ |
| Build System     | CMake 3.21+                   |
| Framework        | Qt 6.5+                       |

### Required Qt Modules

* Qt Core
* Qt Widgets
* Qt Network

### Clone the Repository

```bash
git clone https://github.com/sayedalve/netspeedmeter.git
cd netspeedmeter
```

### Configure the Project

```cmd
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 17 2022" -A x64
```

### Build the Application

```cmd
cmake --build build/release --config Release --parallel
```

---

## 📦 Deployment

Deploy the required Qt runtime libraries using:

```cmd
windeployqt --no-translations --no-compiler-runtime build\release\Release\NetSpeedMeter.exe
```

This creates a standalone executable package that can run on systems without a local Qt installation.

---

## ⚙️ Configuration

Application settings are stored in:

```text
%APPDATA%\NetSpeedMeter\config.json
```

Stored settings include:

* Window position
* Display preferences
* Typography settings
* Scale factors
* Unit modes
* Opacity values

### Reset Configuration

To restore default settings:

1. Close NetSpeedMeter.
2. Navigate to:

```text
%APPDATA%\NetSpeedMeter\
```

3. Delete `config.json`.
4. Restart the application.

---

## 🏗️ Technology Stack

* C++17
* Qt 6
* Windows API
* CMake
* QThread
* QSharedMemory
* GetIfEntry2 Network Interface APIs

---

## 👤 Author

### Md Sayed Alve

Core Architecture & Design

GitHub: https://github.com/sayedalve

---

