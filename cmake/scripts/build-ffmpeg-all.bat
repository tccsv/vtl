@echo off
REM Wrapper: собирает все 6 FFmpeg компонентов в правильном порядке.
REM Применяет все patches (vcxproj, files.props, config.h, lists.c) перед сборкой.

set "ROOT=%~1"
if "%ROOT%"=="" (echo ERROR: project root not given & exit /b 2)

set "SCRIPTS=%ROOT%\cmake\scripts"
set "SMP=%ROOT%\external_sources\ffmpeg\SMP"

REM 1. zlib (нужен FFmpeg для PNG/zlib_wrapper когда CONFIG_ZLIB=1; пока выключен)
REM Раскомментируй если нужно zlib:
REM call "%SCRIPTS%\build-zlib.bat" "%ROOT%" || exit /b 3

REM 2. Apply patches (идемпотентны)
powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPTS%\patch-ffmpeg-vcxproj.ps1" -SmpDir "%SMP%" -VsnasmDir "%ROOT%\external_sources\tools\vsnasm" || exit /b 4
powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPTS%\patch-ffmpeg-config.ps1" -ConfigPath "%SMP%\config.h" || exit /b 5
powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPTS%\patch-ffmpeg-components.ps1" -ConfigComponentsPath "%SMP%\config_components.h" || exit /b 5
powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPTS%\patch-ffmpeg-files.ps1" -SmpDir "%SMP%" || exit /b 6
powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPTS%\patch-ffmpeg-libs.ps1" -SmpDir "%SMP%" || exit /b 7
powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPTS%\patch-ffmpeg-lists.ps1" -SmpDir "%SMP%" || exit /b 8

REM 3. Build components in dependency order
for %%c in (libavutil libswresample libswscale libavcodec libavformat libavfilter) do (
    echo === Building %%c ===
    call "%SCRIPTS%\build-ffmpeg.bat" "%ROOT%" "%%c.vcxproj" || exit /b 9
)

echo FFmpeg built successfully
exit /b 0
