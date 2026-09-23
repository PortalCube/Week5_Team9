@echo off
setlocal

rem Resolve every path from the project root so it works from any directory.
set "PROJECT_DIRECTORY=%~dp0"
set "PREBUILD_SCRIPT=%PROJECT_DIRECTORY%Scripts\PreBuild.ps1"
set "POSTBUILD_SCRIPT=%PROJECT_DIRECTORY%Scripts\PostBuild.ps1"

rem Use the x64 Release output by default. Pass another directory as the first
rem argument to override the target directory.
if "%~1"=="" (
    set "TARGET_DIRECTORY=%PROJECT_DIRECTORY%Binaries\x64\Release"
) else (
    set "TARGET_DIRECTORY=%~1"
)

echo [BuildContent] Running PreBuild scripts...
powershell -NoProfile -ExecutionPolicy Bypass -File "%PREBUILD_SCRIPT%"
if errorlevel 1 (
    echo [BuildContent] PreBuild failed.
    exit /b 1
)

echo [BuildContent] Running PostBuild scripts...
powershell -NoProfile -ExecutionPolicy Bypass -File "%POSTBUILD_SCRIPT%" -TargetDirectory "%TARGET_DIRECTORY%"
if errorlevel 1 (
    echo [BuildContent] PostBuild failed.
    exit /b 1
)

echo [BuildContent] Content update completed: "%TARGET_DIRECTORY%\Content"
exit /b 0
