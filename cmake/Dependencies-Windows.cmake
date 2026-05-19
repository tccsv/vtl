# Windows-сборка под MSVC. Всё внешнее — из исходников.
#
# Что и откуда (всё в external_sources/, собирается при cmake --build):
#   curl   — родной CMake-build с Schannel TLS (native Windows, без OpenSSL)
#   libpq  — собственный CMakeLists.txt в cmake/libpq/, MSVC-only,
#            никаких meson/ninja/Python (pre-generated headers в cmake/libpq/generated/)
#   FFmpeg — ShiftMediaProject 7.1 (msbuild .vcxproj), отключены опциональные
#            кодеки требующие внешних SDK (libx264/libvpx/libmp3lame/...)
#
# Требования к машине (всё кроме CMake/VS лежит в репо):
#   - Visual Studio (MSVC v145+) — для cl.exe / link.exe
#   - CMake 3.20+
# Никакого Python / meson / perl / других interpreter'ов.

if(WIN32 AND NOT MSVC)
    message(FATAL_ERROR
        "Под Windows проект собирается только MSVC (Visual Studio). "
        "MinGW/MSYS2/Cygwin не поддерживаются.")
endif()

include(ExternalProject)

# ============================================================
# curl — source build из external_sources/curl/
# Используем Schannel (нативный Windows TLS, не нужен OpenSSL)
# ============================================================
set(BUILD_SHARED_LIBS    OFF CACHE BOOL "" FORCE)
set(BUILD_CURL_EXE       OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING        OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES       OFF CACHE BOOL "" FORCE)
set(BUILD_LIBCURL_DOCS   OFF CACHE BOOL "" FORCE)
set(ENABLE_CURL_MANUAL   OFF CACHE BOOL "" FORCE)
set(CURL_DISABLE_INSTALL ON  CACHE BOOL "" FORCE)
set(CURL_DISABLE_DOCS    ON  CACHE BOOL "" FORCE)
set(CURL_USE_SCHANNEL    ON  CACHE BOOL "" FORCE)
set(CURL_USE_OPENSSL     OFF CACHE BOOL "" FORCE)
set(CURL_USE_LIBSSH2     OFF CACHE BOOL "" FORCE)
set(CURL_USE_LIBSSH      OFF CACHE BOOL "" FORCE)
set(CURL_USE_LIBPSL      OFF CACHE BOOL "" FORCE)
set(CURL_USE_LIBIDN2     OFF CACHE BOOL "" FORCE)
set(USE_LIBRTMP          OFF CACHE BOOL "" FORCE)
set(USE_NGHTTP2          OFF CACHE BOOL "" FORCE)
set(CURL_ZLIB            OFF CACHE BOOL "" FORCE)
set(CURL_BROTLI          OFF CACHE BOOL "" FORCE)
set(CURL_ZSTD            OFF CACHE BOOL "" FORCE)

add_subdirectory(
    "${CMAKE_SOURCE_DIR}/external_sources/curl"
    "${CMAKE_BINARY_DIR}/curl-build"
    EXCLUDE_FROM_ALL
)

# curl 8.x target = libcurl_static (BUILD_SHARED_LIBS=OFF). Заворачиваем в CURL::libcurl alias.
if(NOT TARGET CURL::libcurl)
    add_library(CURL::libcurl ALIAS libcurl_static)
endif()

# ============================================================
# FFmpeg — source build из external_sources/ffmpeg (ShiftMediaProject 7.1)
# Собирается через msbuild + nasm (см. cmake/scripts/build-ffmpeg-all.bat).
# Артефакты выходят в ${CMAKE_SOURCE_DIR}/msvc/{bin,lib,include}/
# ============================================================
set(FFMPEG_OUT "${CMAKE_SOURCE_DIR}/msvc")
set(FFMPEG_BIN "${FFMPEG_OUT}/bin/x64")
set(FFMPEG_LIB "${FFMPEG_OUT}/lib/x64")
set(FFMPEG_INC "${FFMPEG_OUT}/include")

set(_FFMPEG_DLLS
    "${FFMPEG_BIN}/avutil.dll"
    "${FFMPEG_BIN}/swresample.dll"
    "${FFMPEG_BIN}/swscale.dll"
    "${FFMPEG_BIN}/avcodec.dll"
    "${FFMPEG_BIN}/avformat.dll"
    "${FFMPEG_BIN}/avfilter.dll"
)
set(_FFMPEG_LIBS
    "${FFMPEG_LIB}/avutil.lib"
    "${FFMPEG_LIB}/swresample.lib"
    "${FFMPEG_LIB}/swscale.lib"
    "${FFMPEG_LIB}/avcodec.lib"
    "${FFMPEG_LIB}/avformat.lib"
    "${FFMPEG_LIB}/avfilter.lib"
)

ExternalProject_Add(ffmpeg_external
    SOURCE_DIR        "${CMAKE_SOURCE_DIR}/external_sources/ffmpeg"
    BINARY_DIR        "${CMAKE_SOURCE_DIR}/external_sources/ffmpeg"
    DOWNLOAD_COMMAND  ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND
        cmd /c "${CMAKE_SOURCE_DIR}\\cmake\\scripts\\build-ffmpeg-all.bat"
               "${CMAKE_SOURCE_DIR}"
    INSTALL_COMMAND   ""
    BUILD_BYPRODUCTS  ${_FFMPEG_DLLS} ${_FFMPEG_LIBS}
    BUILD_ALWAYS      OFF
)

add_library(ffmpeg INTERFACE)
target_include_directories(ffmpeg INTERFACE "${FFMPEG_INC}")
add_dependencies(ffmpeg ffmpeg_external)

macro(_vtl_win_ffmpeg comp)
    add_library(ffmpeg_${comp} SHARED IMPORTED GLOBAL)
    set_target_properties(ffmpeg_${comp} PROPERTIES
        IMPORTED_LOCATION "${FFMPEG_BIN}/${comp}.dll"
        IMPORTED_IMPLIB   "${FFMPEG_LIB}/${comp}.lib"
    )
    add_dependencies(ffmpeg_${comp} ffmpeg_external)
    target_link_libraries(ffmpeg INTERFACE ffmpeg_${comp})
endmacro()

_vtl_win_ffmpeg(avcodec)
_vtl_win_ffmpeg(avformat)
_vtl_win_ffmpeg(avutil)
_vtl_win_ffmpeg(avfilter)
_vtl_win_ffmpeg(swscale)
_vtl_win_ffmpeg(swresample)

# ============================================================
# libpq — source build из external_sources/libpq/ через cmake/libpq/CMakeLists.txt.
# MSVC компилирует .c напрямую — без meson, ninja, Python, perl. Pre-generated
# headers (kwlist_d.h / pg_config*.h) в cmake/libpq/generated/ — однократный
# вывод meson зафиксированный в репо (аналогично SMP/config.h у FFmpeg).
#
# Target наружу: libpq (SHARED) + alias PostgreSQL::PostgreSQL.
# DLL ложится прямо в CMAKE_RUNTIME_OUTPUT_DIRECTORY (= app/).
# ============================================================
add_subdirectory(
    "${CMAKE_SOURCE_DIR}/cmake/libpq"
    "${CMAKE_BINARY_DIR}/libpq-build"
)

# ============================================================
# Копирование собранных FFmpeg DLLs в app/.
# curl линкуется статически, libpq собирается напрямую в app/ через RUNTIME_OUTPUT_DIRECTORY.
# ============================================================
file(MAKE_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# FFmpeg DLLs нужно скопировать после их сборки. Делаем через POST_BUILD на отдельной
# цели, чтобы dependency-chain работал: VTL.exe ← copy_ffmpeg_dlls ← ffmpeg_external.
add_custom_target(copy_ffmpeg_dlls ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${_FFMPEG_DLLS}
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/"
    DEPENDS ffmpeg_external
    COMMENT "Копируем FFmpeg DLLs в app/"
)

message(STATUS "VTL Windows: всё из source (curl, libpq, FFmpeg)")
