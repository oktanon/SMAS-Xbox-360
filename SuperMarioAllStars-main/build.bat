@echo off
rem Builds the Super Mario All-Stars PC port (Release) with Visual Studio 2022 + CMake.
setlocal
cd /d "%~dp0"
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 || goto :fail
cmake --build build --config Release || goto :fail
echo.
echo Built: build\Release\SuperMarioAllStars.exe
exit /b 0
:fail
echo Build failed.
exit /b 1
