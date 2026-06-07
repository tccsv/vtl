@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion
title VTL VK bot: build and run main_schulgin

rem Repo root = two levels up from tools\vkbot\
pushd "%~dp0..\.."

echo ============================================================
echo   VTL VK bot - build ^& run main_schulgin
echo ============================================================

rem 1) MSVC compiler: from PATH, else portable C:\BuildTools, else auto-detect via vswhere
where cl >nul 2>nul
if not errorlevel 1 (
  echo [env] cl already in PATH
  goto :have_cl
)
if exist "C:\BuildTools\devcmd.bat" (
  call "C:\BuildTools\devcmd.bat" >nul
  echo [env] portable MSVC from C:\BuildTools
  goto :have_cl
)
rem Auto-detect an installed Visual Studio / Build Tools and load its environment.
rem vcvars64.bat brings cl, cmake and ninja at once, so the checks below pass too.
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSPATH="
if exist "%VSWHERE%" for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%I"
if defined VSPATH if exist "!VSPATH!\VC\Auxiliary\Build\vcvars64.bat" (
  call "!VSPATH!\VC\Auxiliary\Build\vcvars64.bat" >nul
  echo [env] MSVC auto-detected: "!VSPATH!"
  goto :have_cl
)
echo [env] cl not found - open "Developer Command Prompt for VS" or install MSVC
:have_cl

rem 2) CMake/Ninja: from PATH, else portable from %USERPROFILE%\vtl-toolchain
where cmake >nul 2>nul
if errorlevel 1 (
  for /d %%D in ("%USERPROFILE%\vtl-toolchain\cmake\cmake-*-windows-x86_64") do set "CMK=%%D\bin"
  if defined CMK set "PATH=!CMK!;%USERPROFILE%\vtl-toolchain\ninja;!PATH!"
)
where cmake >nul 2>nul
if errorlevel 1 (
  echo [ERROR] cmake not found. Install CMake 3.20+.
  goto :end
)

set "GEN="
where ninja >nul 2>nul && set "GEN=-G Ninja"

echo.
echo [1/3] Configure...
cmake -S . -B build %GEN% -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 ( echo [ERROR] configure failed & goto :end )

echo.
echo [2/3] Build main_schulgin...
cmake --build build --target main_schulgin
if errorlevel 1 ( echo [ERROR] build failed & goto :end )

echo.
rem Community access token. Always ask on launch; Enter keeps an already-set value.
rem (Settings -^> Manage -^> API usage -^> Access keys)
echo Enter the VK community API key (access token).
set "VK_KEY_INPUT="
if defined VK_BOT_TOKEN (
  set /p "VK_KEY_INPUT=API key [Enter to keep current]: "
) else (
  set /p "VK_KEY_INPUT=API key: "
)
if defined VK_KEY_INPUT set "VK_BOT_TOKEN=!VK_KEY_INPUT!"
if not defined VK_BOT_TOKEN (
  echo [ERROR] API key not entered - cannot start the bot.
  goto :end
)

rem Numeric community id (without minus). Long Poll API must be enabled in community settings.
if not defined VK_GROUP_ID (
  set /p "VK_GROUP_ID=Paste VK community (group) id and press Enter: "
)

echo.
echo [3/3] Run...
echo ------------------------------------------------------------
app\main_schulgin.exe
echo ------------------------------------------------------------

:end
echo.
popd
pause
endlocal
