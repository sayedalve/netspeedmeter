# NetSpeedMeter C++ — Batch 1: Core Networking & Config

## What was built

| File | Purpose |
|---|---|
| `CMakeLists.txt` | Full CMake project: Qt6, iphlpapi, WIN32 subsystem, Release LTO |
| `src/core/SpeedSample.h` | Plain value type: `uploadBps`, `downloadBps` + unit helpers |
| `src/core/NetworkPoller.h/.cpp` | `QThread`-based poller using Windows `GetIfEntry2()` |
| `src/config/AppConfig.h` | Pure data struct with every configurable value and defaults |
| `src/config/ConfigManager.h/.cpp` | Singleton, JSON load/save via `QJsonDocument` |
| `src/main.cpp` | Batch 1 smoke-test harness (debug print of speeds) |
| `build_debug.bat` | One-click Debug build |
| `build_release.bat` | One-click Release build |

---

## Architecture decisions

### NetworkPoller — why a QThread instead of QTimer?

`QTimer` fires on the GUI thread. Network I/O (even a simple `GetIfEntry2` call)
can block for milliseconds under load. Running it on a dedicated `QThread` keeps
the GUI perfectly responsive even at 100 ms poll intervals.

The poller sleeps in 50 ms increments so `stop()` is acknowledged within ≤ 50 ms,
rather than having to wait for the full interval timeout.

### Delta-over-elapsed-time instead of fixed interval

The actual sleep duration varies slightly due to OS scheduling. Dividing the byte
delta by the *measured* elapsed time (via `QElapsedTimer`) gives accurate B/s
rather than one that drifts proportionally to the jitter.

### iphlpapi GetIfEntry2 instead of Qt network classes

`QNetworkInterface` exposes adapter names and addresses but not traffic counters.
`GetIfEntry2` (Vista+) returns 64-bit `InOctets` / `OutOctets` per adapter —
exactly what we need, with no periodic counter wrap until ≈ 18 exabytes.

### ConfigManager — singleton with explicit save

Auto-save on every `setConfig()` call would create spurious disk writes during
batch settings changes. The explicit `save()` call (triggered on dialog Accept /
application exit) groups all changes into a single write.

---

## How to verify Batch 1

### Prerequisites

- Qt 6.5+ installed with MSVC or MinGW toolchain
- CMake 3.21+
- MSVC 2019/2022 or MinGW 12+

### Build

```bat
REM Set your Qt path first (skip if it's already on PATH / cmake finds it)
set QT_DIR=C:\Qt\6.7.0\msvc2022_64

build_debug.bat
```

### Run

```bat
build\debug\Debug\NetSpeedMeter.exe
```

You should see output similar to:

```
=== Detected Network Adapters ===
 [ 5 ] Intel(R) Wi-Fi 6E AX211 160MHz
 [ 7 ] Realtek PCIe GbE Family Controller
================================
Active adapter: Intel(R) Wi-Fi 6E AX211 160MHz  [ {GUID} ]
↑ 0.3 KB/s  ↓ 4.7 KB/s
↑ 0.1 KB/s  ↓ 12.3 KB/s
...
```

A `config.json` will be written to `%APPDATA%\NetSpeedMeter\NetSpeedMeter\config.json`
on clean exit (Ctrl+C / close window).

---

## What comes next

| Batch | Content |
|---|---|
| **2** | `SettingsDialog` — tabbed QDialog (General / Network / Appearance), reads & writes `AppConfig` |
| **3** | `TrayManager` — system tray icon, right-click menu, links dialog, keeps app alive (replaces the keepAlive timer in `main.cpp`) |
| **4** | `OverlayWidget` — frameless transparent QWidget, mouse drag, position persistence |
| **5** | `SpeedRenderer` — QPainter engine: typography, arrows, mini area-chart history graph |

---

## Key design invariants to preserve in later batches

1. **`NetworkPoller` is never touched from the GUI thread** except through its
   thread-safe setters (`setIntervalMs`, `setAdapterMode`, `setSelectedAdapters`).

2. **`ConfigManager::save()`** is always called either on Settings dialog Accept
   or `QApplication::aboutToQuit` — never more frequently.

3. The `nsm::` namespace wraps all project classes to prevent name collisions with
   Qt / Windows SDK symbols.

4. **No raw `new` without an owner** — every heap allocation uses a Qt parent,
   `std::unique_ptr`, or RAII container.
