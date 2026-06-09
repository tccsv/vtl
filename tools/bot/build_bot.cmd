@echo off
chcp 65001 >nul
title VTL Telegram bot - build

rem Корень репозитория = на два уровня выше tools\bot\
cd /d "%~dp0..\.."

rem --- Окружение компилятора (MSVC) ---
rem 1) если cl уже в PATH (запущен из "x64 Native Tools Command Prompt") — ок;
rem 2) иначе пробуем портативный C:\BuildTools\devcmd.bat.
where cl >nul 2>nul
if errorlevel 1 (
  if exist "C:\BuildTools\devcmd.bat" (
    call "C:\BuildTools\devcmd.bat" >nul 2>&1
  ) else (
    echo [ОШИБКА] cl не найден. Открой "x64 Native Tools Command Prompt for VS"
    echo или установи компилятор MSVC.
    pause & exit /b 1
  )
)

rem --- CMake/Ninja: из PATH, иначе портативные из %USERPROFILE%\vtl-toolchain ---
where cmake >nul 2>nul
if errorlevel 1 (
  for /d %%D in ("%USERPROFILE%\vtl-toolchain\cmake\cmake-*-windows-x86_64") do set "CMK=%%D\bin"
  if defined CMK set "PATH=!CMK!;%USERPROFILE%\vtl-toolchain\ninja;%PATH%"
)
where cmake >nul 2>nul || ( echo [ОШИБКА] cmake не найден. & pause & exit /b 1 )

set "GEN="
where ninja >nul 2>nul && set "GEN=-G Ninja"

echo [1/3] Конфигурация...
cmake -S . -B build %GEN% -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 ( echo [ОШИБКА] configure & pause & exit /b 1 )

echo [2/3] Сборка main_firyulin (в первый раз собирается FFmpeg, ~10-15 мин)...
cmake --build build --target main_firyulin
if errorlevel 1 ( echo [ОШИБКА] build & pause & exit /b 1 )

echo [3/3] Копирование FFmpeg DLL в app\...
if exist "msvc\bin\x64\avcodec.dll" copy /y "msvc\bin\x64\*.dll" "app\" >nul 2>nul

echo.
echo [ГОТОВО] app\main_firyulin.exe собран. Запуск: tools\bot\run_bot.cmd
pause
