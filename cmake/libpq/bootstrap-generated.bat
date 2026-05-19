@echo off
REM Bootstrap: одноразовая регенерация generated headers libpq через meson.
REM Запускается ВРУЧНУЮ на машине разработчика когда обновляется postgres source
REM в external_sources/libpq/. Результат — headers в cmake/libpq/generated/ —
REM фиксируется в git. После этого CMake-сборка не нуждается в Python/meson.
REM
REM Usage: cmake\libpq\bootstrap-generated.bat
REM Требует: Python 3.x с meson (`py -m pip install meson`).

setlocal

set "ROOT=%~dp0..\.."
set "ROOT=%ROOT:\=/%"
set "SRC=%ROOT%/external_sources/libpq"
set "BUILD=%ROOT%/build-source/libpq-bootstrap"
set "GEN=%ROOT%/cmake/libpq/generated"

set "VSWHERE=C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -property installationPath`) do set "VS=%%i"
call "%VS%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 (echo ERROR: vcvars64 failed & exit /b 3)

for /f "usebackq tokens=*" %%i in (`py -c "import sysconfig; print(sysconfig.get_path('scripts'))"`) do set "PYSCRIPTS=%%i"
set "WFB=%ROOT:/=\%\external_sources\tools\winflexbison"
set "PATH=%PYSCRIPTS%;%WFB%;%PATH%"

if exist "%BUILD%" rmdir /S /Q "%BUILD%"

meson setup "%BUILD%" "%SRC%" ^
    --buildtype=release ^
    -Dnls=disabled -Dssl=none -Dzlib=disabled -Dauto_features=disabled ^
    -Dplperl=disabled -Dplpython=disabled -Dpltcl=disabled ^
    -Dicu=disabled -Dlz4=disabled -Dzstd=disabled ^
    -Dlibxml=disabled -Dlibxslt=disabled -Dreadline=disabled ^
    -Duuid=none -Dldap=disabled -Dgssapi=disabled ^
    -Dsystemd=disabled -Dpam=disabled -Dbsd_auth=disabled ^
    -Dbonjour=disabled -Dselinux=disabled -Dtap_tests=disabled -Ddtrace=disabled
if errorlevel 1 (echo ERROR: meson setup failed & exit /b 2)

REM Gen headers: kwlist_d.h / errcodes.h рождаются только при build (custom_target),
REM поэтому запускаем ninja на тех целях которые нам нужны. Полный build не нужен.
REM Полный build — нужен ради side-effect'а: meson при build генерит kwlist_d.h,
REM errcodes.h и catalog/*_d.h как зависимости целевых .obj. Артефакт libpq.dll
REM мы игнорируем — финальный билд через cmake/libpq/CMakeLists.txt.
meson compile -C "%BUILD%"
if errorlevel 1 (echo ERROR: meson compile failed & exit /b 4)

REM Headers нужные libpq из meson-вывода. Копируем в cmake/libpq/generated/.
if not exist "%GEN%\include\utils"   mkdir "%GEN%\include\utils"
if not exist "%GEN%\include\catalog" mkdir "%GEN%\include\catalog"
if not exist "%GEN%\include\nodes"   mkdir "%GEN%\include\nodes"
if not exist "%GEN%\common"          mkdir "%GEN%\common"

copy /Y "%BUILD%\src\include\pg_config.h"        "%GEN%\include\"        >nul
copy /Y "%BUILD%\src\include\pg_config_os.h"     "%GEN%\include\"        >nul
copy /Y "%BUILD%\src\include\pg_config_paths.h"  "%GEN%\include\"        >nul
copy /Y "%BUILD%\src\common\kwlist_d.h"          "%GEN%\common\"         >nul
copy /Y "%BUILD%\src\include\utils\errcodes.h"   "%GEN%\include\utils\"  >nul
copy /Y "%BUILD%\src\include\catalog\*.h"        "%GEN%\include\catalog\" >nul
copy /Y "%BUILD%\src\include\nodes\nodetags.h"   "%GEN%\include\nodes\"  >nul 2>&1

echo Generated headers updated in cmake\libpq\generated\
echo Commit them to git so other developers do not need Python+meson.
endlocal & exit /b 0
