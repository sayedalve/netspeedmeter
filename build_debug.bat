@echo off
setlocal

REM ============================================================
REM  NetSpeedMeter — Debug Build Helper
REM  Prerequisites: Qt6, CMake 3.21+, MSVC or MinGW
REM ============================================================

REM Set this to your Qt6 installation path if cmake cannot find it automatically
REM e.g.: set QT_DIR=C:\Qt\6.7.0\msvc2022_64
if defined QT_DIR (
    set CMAKE_PREFIX_PATH=%QT_DIR%
)

set BUILD_DIR=%~dp0build\debug

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

echo [1/2] Configuring (Debug) ...
cmake -S "%~dp0" ^
      -B "%BUILD_DIR%" ^
      -DCMAKE_BUILD_TYPE=Debug ^
      -G "Visual Studio 17 2022" ^
      -A x64
if ERRORLEVEL 1 (
    echo [ERROR] CMake configuration failed.
    pause
    exit /b 1
)

echo [2/2] Building ...
cmake --build "%BUILD_DIR%" --config Debug --parallel
if ERRORLEVEL 1 (
    echo [ERROR] Build failed.
    pause
    exit /b 1
)

echo.
echo [OK] Debug build complete: %BUILD_DIR%\Debug\NetSpeedMeter.exe
pause
