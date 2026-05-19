@echo off
setlocal
REM Usage: build-ffmpeg.bat <PROJECT_ROOT> <VCXPROJ_NAME> [Configuration]
REM PROJECT_ROOT — корень репо VTL.
REM VCXPROJ_NAME — например "libavutil.vcxproj".
REM Configuration — по умолчанию ReleaseDLL (даст shared DLL+lib).

set "ROOT=%~1"
set "PROJ=%~2"
set "CFG=%~3"
if "%CFG%"=="" set "CFG=ReleaseDLL"

if "%ROOT%"=="" (echo ERROR: project root not given & exit /b 2)
if "%PROJ%"=="" (echo ERROR: vcxproj name not given & exit /b 2)

set "VSWHERE=C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (echo ERROR: vswhere.exe not found & exit /b 3)
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -property installationPath`) do set "VS_PATH=%%i"
if "%VS_PATH%"=="" (echo ERROR: Visual Studio not found & exit /b 3)

call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 (echo ERROR: vcvars64 failed & exit /b 4)

REM Локальный NASM в PATH (нужен для .asm сборки в FFmpeg)
set "PATH=%ROOT%\external_sources\tools\nasm;%PATH%"

REM Проверка
nasm --version >nul 2>&1
if errorlevel 1 (echo ERROR: nasm.exe not found in PATH & exit /b 5)

cd /d "%ROOT%\external_sources\ffmpeg\SMP"
msbuild "%PROJ%" /p:Configuration=%CFG% /p:Platform=x64 /m /verbosity:minimal /nologo
set RC=%ERRORLEVEL%
endlocal & exit /b %RC%
