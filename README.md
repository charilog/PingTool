![PingTool Screenshot](PingTool.png)

# PingTool (C++/Qt)

A lightweight Qt6 Widgets app for basic network diagnostics on Windows: **Ping**, **Traceroute**, **DNS lookup**, and **TCP connect test** with live output.

## Requirements
- Windows 10/11
- Qt 6.x (Widgets + Network)
- CMake
- Visual Studio + MSBuild

## Build & Run (PowerShell)
```powershell
cd C:\Users\admin\Desktop\PingTool

Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue

cmake -S . -B build -G "Visual Studio 18 2026" -A x64 `
  -DQt6_DIR="C:\Qt\6.10.1\msvc2022_64\lib\cmake\Qt6" `
  -DCMAKE_GENERATOR_INSTANCE="C:\Program Files\Microsoft Visual Studio\18\Community"

cmake --build build --config Release

& "C:\Qt\6.10.1\msvc2022_64\bin\windeployqt.exe" .\build\Release\PingTool.exe

.\build\Release\PingTool.exe
```

## Usage
- **Host(s):** enter a hostname/IP. Multiple hosts supported (separate with space/comma/semicolon).
- **Ping:** runs `ping` and shows live output.
- **Stop:** terminates the running command.
- **Traceroute:** runs `tracert`.
- **DNS:** forward/reverse lookup via Qt.
- **TCP Test:** connects to host:**TCP Port** and reports result/latency.
- **Copy / Save / Clear:** manage the output log.

## Notes
- If ICMP is blocked, use **TCP Test** (default port 443).
