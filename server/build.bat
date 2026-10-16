@echo off
rem ============================================================
rem Blog server - Windows build script (MSVC + CMake)
rem Usage: run build.bat inside server/ dir
rem Output: build\Release\blog_server.exe
rem ============================================================
setlocal

set VCVARS="D:\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist %VCVARS% (
  echo [ERR] vcvars64.bat not found. Edit VCVARS path in build.bat
  exit /b 1
)
call %VCVARS% >nul

echo [1/2] cmake configure ...
cmake -S . -B build
if errorlevel 1 exit /b 1

echo [2/2] cmake build Release ...
cmake --build build --config Release
if errorlevel 1 exit /b 1

echo [OK] output: build\Release\blog_server.exe
endlocal
