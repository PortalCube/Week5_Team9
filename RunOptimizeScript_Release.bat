@echo off
setlocal

rem Resolve every path from the project root so it works from any directory.
set "PROJECT_DIRECTORY=%~dp0"
set "OPTIMIZE_SCRIPT=%PROJECT_DIRECTORY%Scripts\Optimize-Content.ps1"
set "TARGET_DIRECTORY=%PROJECT_DIRECTORY%Binaries\x64\Release"

echo [OptimizeContent] Optimizing "%TARGET_DIRECTORY%\Content"...
powershell -NoProfile -ExecutionPolicy Bypass -File "%OPTIMIZE_SCRIPT%" -TargetDirectory "%TARGET_DIRECTORY%"
if errorlevel 1 (
    echo [OptimizeContent] Optimization failed.
    exit /b 1
)

echo [OptimizeContent] Optimization completed: "%TARGET_DIRECTORY%\Content"
exit /b 0
