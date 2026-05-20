# Подключает зависимости из external_libs/ как IMPORTED CMake-таргеты.
# Никаких find_package / pkg-config — всё локально в репо.
# Цель: проект собирается на чистой ОС без apt/brew/vcpkg.
#
# Поддерживаемые платформы:
#   Linux x86_64      → external_libs/{curl,postgresql}/lib/*.so
#   Windows x86_64    → собирается из external_sources/ под MSVC
#   macOS arm64       → external_libs/macos/lib/*.dylib

set(EXTERNAL_LIBS_DIR "${CMAKE_SOURCE_DIR}/external_libs")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    include("${CMAKE_CURRENT_LIST_DIR}/Dependencies-Windows.cmake")
    return()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    include("${CMAKE_CURRENT_LIST_DIR}/Dependencies-MacOS.cmake")
    return()
endif()

if(NOT (CMAKE_SYSTEM_NAME STREQUAL "Linux"))
    message(FATAL_ERROR
            "external_libs/ под ${CMAKE_SYSTEM_NAME} пока нет. "
            "Поддерживается Linux x86_64, Windows x86_64 (MSVC), macOS arm64.")
endif()

# Создаём недостающие файлы .so.MAJOR — физической копией .so.MAJOR.MINOR.PATCH.
# Они нужны для runtime: линкер записывает в DT_NEEDED soname библиотеки,
# а в репо изначально лежит только версионированный файл.
function(_vtl_ensure_soname dir versioned soname_name)
    set(target "${dir}/${soname_name}")
    if(NOT EXISTS "${target}")
        configure_file("${dir}/${versioned}" "${target}" COPYONLY)
    endif()
endfunction()

# ============================================================
# FFmpeg — используем системные пакеты через pkg-config
# ============================================================
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
        libavcodec
        libavformat
        libavutil
        libavfilter
        libswscale
        libswresample
)

add_library(ffmpeg INTERFACE)
target_link_libraries(ffmpeg INTERFACE PkgConfig::FFMPEG)

# ============================================================
# libcurl — используем системный пакет
# ============================================================
find_package(CURL REQUIRED)

# ============================================================
# libpq (PostgreSQL) — используем системный пакет
# ============================================================
find_package(PostgreSQL REQUIRED)

# ============================================================
# RPATH
# ============================================================
# Бинарь app/VTL должен находить .so в external_libs/<pkg>/lib/ во время запуска.
# Используем $ORIGIN — относительный путь от бинаря.
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "")

message(STATUS "VTL dependencies: FFmpeg/curl/libpq restored via system packages.")