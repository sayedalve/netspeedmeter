@echo off
setlocal

REM ============================================================
REM  NetSpeedMeter — Release Build Helper
REM  Produces a compact, optimised executable in build\release\
REM ============================================================

if defined QT_DIR (
    set CMAKE_PREFIX_PATH=%QT_DIR%
)

set BUILD_DIR=%~dp0build\release

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

echo [1/2] Configuring (Release) ...
cmake -S "%~dp0" ^
      -B "%BUILD_DIR%" ^
      -DCMAKE_BUILD_TYPE=Release ^
      -G "Visual Studio 17 2022" ^
      -A x64
if ERRORLEVEL 1 (
    echo [ERROR] CMake configuration failed.
    pause
    exit /b 1
)

echo [2/2] Building ...
cmake --build "%BUILD_DIR%" --config Release --parallel
if ERRORLEVEL 1 (
    echo [ERROR] Build failed.
    pause
    exit /b 1
)

echo.
echo [OK] Release build complete.
echo      Executable: %BUILD_DIR%\Release\NetSpeedMeter.exe
echo      Run windeployqt manually if the auto-step was skipped.
pause
