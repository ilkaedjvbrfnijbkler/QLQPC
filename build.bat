@echo off
setlocal enableextensions enabledelayedexpansion
cd /d "%~dp0"

set "DO_CLEAN=0"
set "DO_DEPLOY=1"
set "CFG=Release"

:parse_args
if "%~1"=="" goto :args_done
if /i "%~1"=="clean"    (set "DO_CLEAN=1"  & shift & goto :parse_args)
if /i "%~1"=="debug"    (set "CFG=Debug"   & shift & goto :parse_args)
if /i "%~1"=="nodeploy" (set "DO_DEPLOY=0" & shift & goto :parse_args)
echo Usage: build.bat [clean] [debug] [nodeploy]
exit /b 1
:args_done

if "%DO_CLEAN%"=="1" (
    if exist build rmdir /s /q build
)
if not exist build mkdir build

where cl >nul 2>nul
if errorlevel 1 goto :init_vs
if /i "%VSCMD_ARG_TGT_ARCH%"=="x86" goto :vs_ready

:init_vs
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [build] ERROR: vswhere.exe not found. Install Visual Studio 2022 or Build Tools.
    exit /b 1
)
set "VS_DIR="
for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS_DIR=%%I"
if not defined VS_DIR (
    echo [build] ERROR: Visual Studio C++ x86/x64 tools not found.
    exit /b 1
)
call "%VS_DIR%\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul
if errorlevel 1 exit /b 1

:vs_ready

if /i "%CFG%"=="Debug" (
    set "CFLAGS_OPT=/Od /Z7 /RTC1 /MTd /D_DEBUG"
    set "LDFLAGS_CFG=/DEBUG"
) else (
    set "CFLAGS_OPT=/O2 /Oi /MT /DNDEBUG"
    set "LDFLAGS_CFG=/DEBUG:NONE /NOCOFFGRPINFO"
)

set "CFLAGS_BASE=/nologo /std:c++20 /EHsc /DWIN32_LEAN_AND_MEAN /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS /Isrc /Ibuild\gen"
set "CFLAGS_WARN=/W4 /permissive- /sdl"

rem
powershell -NoProfile -ExecutionPolicy Bypass -File tools\gen_webui_assets.ps1
if errorlevel 1 exit /b 1

if exist build\obj rmdir /s /q build\obj
mkdir build\obj

rem
echo [build] Building dinput8.dll ^(%CFG%^)...
cl %CFLAGS_BASE% %CFLAGS_OPT% %CFLAGS_WARN% /MP /Fobuild\obj\ ^
    src\*.cpp ^
    src\proxy\*.cpp ^
    src\game\*.cpp ^
    src\cvars\*.cpp ^
    src\visuals\*.cpp ^
    src\util\*.cpp ^
    src\timing\*.cpp ^
    src\hitreg\*.cpp ^
    src\webui\*.cpp ^
    src\vulkan\*.cpp ^
    /link /NOLOGO /DLL %LDFLAGS_CFG% /DEF:src\proxy\dinput8.def /MANIFEST:NO /BREPRO /OUT:build\dinput8.dll ^
    winmm.lib user32.lib gdi32.lib advapi32.lib
if errorlevel 1 goto :fail

rmdir /s /q build\obj
del /q build\dinput8.lib build\dinput8.exp >nul 2>nul

rem
if "%DO_DEPLOY%"=="1" (
    powershell -NoProfile -ExecutionPolicy Bypass -File tools\deploy.ps1 -SourceDir build -Config "%CFG%" -Force
    if errorlevel 1 exit /b 1
)

exit /b 0

:fail
if exist build\obj rmdir /s /q build\obj
exit /b 1
