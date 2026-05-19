@echo off
REM Usage: build-zlib.bat <PROJECT_ROOT>
REM Собирает zlib как static lib и кладёт zlib.lib + zlib.h + zconf.h
REM в msvc/lib/x64 и msvc/include — там где FFmpeg vcxproj их ищет.

set "ROOT=%~1"
if "%ROOT%"=="" (echo ERROR: project root not given & exit /b 2)

set "VSWHERE=C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -property installationPath`) do set "VS_PATH=%%i"
call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" >nul

set "SRC=%ROOT%\external_sources\zlib"
set "BUILD=%ROOT%\external_sources\.zlib-build"
set "MSVC=%ROOT%\msvc"

REM Нужно очищать build при смене флагов (LTCG, etc.) — иначе CMake кеширует.
if exist "%BUILD%\CMakeCache.txt" rmdir /S /Q "%BUILD%"

cmake -S "%SRC%" -B "%BUILD%" -A x64 ^
    -D BUILD_SHARED_LIBS=OFF -D ZLIB_BUILD_EXAMPLES=OFF ^
    -D CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL
if errorlevel 1 (echo ERROR: cmake configure failed & exit /b 3)

cmake --build "%BUILD%" --config Release --target zlibstatic
if errorlevel 1 (echo ERROR: cmake build failed & exit /b 4)

REM Раскладка артефактов в msvc/ layout что ожидает FFmpeg vcxproj
if not exist "%MSVC%\include" mkdir "%MSVC%\include"
if not exist "%MSVC%\lib\x64" mkdir "%MSVC%\lib\x64"

copy /Y "%SRC%\zlib.h"            "%MSVC%\include\zlib.h"   >nul
copy /Y "%BUILD%\zconf.h"         "%MSVC%\include\zconf.h"  >nul
copy /Y "%BUILD%\Release\zlibstatic.lib" "%MSVC%\lib\x64\zlib.lib" >nul

echo zlib built and placed in %MSVC%
exit /b 0
