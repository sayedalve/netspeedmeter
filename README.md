# NetSpeedMeter

A lightweight, high performance network speed monitor that integrates seamlessly into the Windows taskbar. Built with **C++17** and **Qt 6**, NetSpeedMeter uses native Windows APIs to provide accurate, real time download and upload speed metrics directly within your workspace without unnecessary background clutter.

---

## 🚀 Features

### Native Taskbar Integration

* Frameless, transparent interface designed to blend naturally with the Windows taskbar.
* Advanced Z order locking mechanism prevents the overlay from slipping behind the taskbar during user interaction.
* Minimal footprint with a clean and distraction free appearance.

### Intelligent Visibility Management

* Automatically hides when fullscreen applications are detected.
* Supports games, media playback, presentations, and other fullscreen activities.
* Detects Windows Start Menu and Search panels to prevent overlay conflicts.

### Windows 11 Input Handling

* Custom low level mouse event interception.
* Prevents Windows 11 shell input interception issues.
* Provides a responsive dark themed context menu experience.

### Accurate Network Monitoring

* Uses native Windows **GetIfEntry2** APIs.
* Reads 64 bit network interface byte counters directly from the operating system.
* Delivers precise upload and download speed measurements.

### Single Instance Protection

* Prevents multiple copies of the application from running simultaneously.
* Implemented using **QSharedMemory** safeguards.

### Advanced Customization

* Font family selection.
* Adjustable font sizes and weights.
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

Traditional `QTimer` based solutions execute on the main UI thread, which can introduce interface lag during intensive processing.

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

## 💻 Getting Started

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

---

## 🔨 Build Instructions

### 1. Clone the Repository

```bash
git clone https://github.com/sayedalve/netspeedmeter.git
cd netspeedmeter
```

### 2. Configure the Project

```cmd
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 17 2022" -A x64
```

### 3. Build the Application

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

Application settings are stored locally in:

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

**Md Sayed Alve**

* Core Architecture & Design
* GitHub: https://github.com/sayedalve

---

