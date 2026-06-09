@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion
title VTL Telegram bot - run

rem Корень репозитория = на два уровня выше tools\bot\
cd /d "%~dp0..\.."

set "SECRETS=%~dp0secrets.env"
if not exist "%SECRETS%" (
  echo [ОШИБКА] Нет файла секретов: %SECRETS%
  echo Скопируй secrets.env.example -^> secrets.env и впиши токен:
  echo     copy tools\bot\secrets.env.example tools\bot\secrets.env
  pause & exit /b 1
)

rem Загружаем KEY=VALUE из secrets.env (eol=# пропускает строки-комментарии)
for /f "usebackq eol=# tokens=1,* delims==" %%a in ("%SECRETS%") do set "%%a=%%b"

if "%TG_BOT_TOKEN%"=="" goto :no_token
if "%TG_BOT_TOKEN%"=="PUT_YOUR_BOT_TOKEN_HERE" goto :no_token

rem FFmpeg/libpq DLL должны лежать рядом с exe (в app\)
if exist "msvc\bin\x64\avcodec.dll" copy /y "msvc\bin\x64\*.dll" "app\" >nul 2>nul

if not exist "app\main_firyulin.exe" (
  echo [ОШИБКА] app\main_firyulin.exe не собран.
  echo Собери сначала:  tools\bot\build_bot.cmd
  pause & exit /b 1
)

echo ============================================================
echo   Запуск VTL-бота. Токен загружен из secrets.env.
echo   Ctrl+C — остановить.
echo ============================================================
app\main_firyulin.exe

endlocal
exit /b 0

:no_token
echo [ОШИБКА] В %SECRETS% не задан TG_BOT_TOKEN (там всё ещё заглушка).
pause & exit /b 1
