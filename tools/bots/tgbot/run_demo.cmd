@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion
title VTL Telegram bot: build and run main_firyulin

rem Repo root = three levels up from tools\bots\tgbot\
pushd "%~dp0..\..\.."

echo ============================================================
echo   VTL Telegram bot - build ^& run main_firyulin
echo ============================================================

rem 1) MSVC compiler: from PATH, else portable from C:\BuildTools
where cl >nul 2>nul
if errorlevel 1 (
  if exist "C:\BuildTools\devcmd.bat" (
    call "C:\BuildTools\devcmd.bat" >nul
    echo [env] portable MSVC from C:\BuildTools
  ) else (
    echo [env] cl not found - open "Developer Command Prompt for VS" or install MSVC
  )
) else (
  echo [env] cl already in PATH
)

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
echo [2/3] Build main_firyulin...
cmake --build build --target main_firyulin
if errorlevel 1 ( echo [ERROR] build failed & goto :end )

echo.
if not defined TG_BOT_TOKEN (
  set /p "TG_BOT_TOKEN=Paste @BotFather token and press Enter [Enter = demo run]: "
)

echo.
echo [3/3] Run...
echo ------------------------------------------------------------
app\main_firyulin.exe
echo ------------------------------------------------------------

:end
echo.
popd
pause
endlocal
