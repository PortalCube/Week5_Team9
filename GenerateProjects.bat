@echo off
setlocal

where premake5 >nul 2>nul
if errorlevel 1 (
    echo [ERROR] premake5 was not found in PATH.
    exit /b 1
)

pushd "%~dp0"
premake5 vs2026
set "PREMAKE_EXIT_CODE=%ERRORLEVEL%"
popd

if not "%PREMAKE_EXIT_CODE%"=="0" (
    echo [ERROR] Premake project generation failed.
    exit /b %PREMAKE_EXIT_CODE%
)

echo Visual Studio 2026 projects generated successfully.
endlocal
