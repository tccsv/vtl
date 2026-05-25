# Подключает зависимости из external_libs/ как IMPORTED CMake-таргеты,
# либо собирает их из external_sources/.
# Никаких find_package / pkg-config из системы.
# Цель: проект собирается на чистой ОС без apt/brew/vcpkg.
#
# Поддерживаемые платформы:
#   Linux x86_64   → external_sources/ source-build (см. Dependencies-Linux.cmake)
#   Windows x86_64 → external_sources/ source-build под MSVC (см. Dependencies-Windows.cmake)
#   macOS arm64    → external_libs/macos/lib/*.dylib (см. Dependencies-MacOS.cmake)

set(EXTERNAL_LIBS_DIR "${CMAKE_SOURCE_DIR}/external_libs")
set(EXTERNAL_SOURCES_DIR "${CMAKE_SOURCE_DIR}/external_sources")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    include("${CMAKE_CURRENT_LIST_DIR}/Dependencies-Windows.cmake")
    return()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    include("${CMAKE_CURRENT_LIST_DIR}/Dependencies-MacOS.cmake")
    return()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include("${CMAKE_CURRENT_LIST_DIR}/Dependencies-Linux.cmake")
    return()
endif()

message(FATAL_ERROR
    "Платформа ${CMAKE_SYSTEM_NAME} пока не поддерживается. "
    "Поддерживается Linux x86_64, Windows x86_64 (MSVC), macOS arm64.")
