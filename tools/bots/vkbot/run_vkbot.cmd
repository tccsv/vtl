@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion
title VTL VK bot - run

rem Корень репозитория = на три уровня выше tools\bots\vkbot\
cd /d "%~dp0..\..\.."

set "SECRETS=%~dp0secrets.env"
if not exist "%SECRETS%" (
  echo [ОШИБКА] Нет файла секретов: %SECRETS%
  echo Скопируй secrets.env.example -^> secrets.env и впиши ключ:
  echo     copy tools\bots\vkbot\secrets.env.example tools\bots\vkbot\secrets.env
  pause & exit /b 1
)

rem Загружаем KEY=VALUE из secrets.env (eol=# пропускает комментарии)
for /f "usebackq eol=# tokens=1,* delims==" %%a in ("%SECRETS%") do set "%%a=%%b"

if "%VK_TOKEN%"==""                    goto :no_token
if "%VK_TOKEN%"=="PUT_YOUR_VK_TOKEN_HERE" goto :no_token

rem FFmpeg/libpq DLL рядом с exe
if exist "msvc\bin\x64\avcodec.dll" copy /y "msvc\bin\x64\*.dll" "app\" >nul 2>nul

if not exist "app\main_schulgin.exe" (
  echo [ОШИБКА] app\main_schulgin.exe не собран.
  echo Собери сначала:  tools\bots\vkbot\build_vkbot.cmd
  pause & exit /b 1
)

echo ============================================================
echo   Запуск VK-бота. Ключ/группа загружены из secrets.env.
echo   Ctrl+C — остановить.
echo ============================================================
app\main_schulgin.exe

endlocal
exit /b 0

:no_token
echo [ОШИБКА] В %SECRETS% не задан VK_TOKEN (там всё ещё заглушка).
pause & exit /b 1
