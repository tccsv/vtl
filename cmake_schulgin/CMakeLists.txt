# =============================================================================
# cmake_schulgin — отдельный (standalone) CMake-проект для сборки ТОЛЬКО
# исполняемого файла VK-бота (main_schulgin.exe / main_schulgin).
#
# Зачем отдельно от корневого CMakeLists.txt:
#   Корневой проект на `cmake --build` тянет ещё FFmpeg (~10-15 мин) и libpq
#   (цель copy_ffmpeg_dlls помечена ALL). VK-боту они не нужны — он зависит
#   только от parson (JSON) и libcurl (HTTP). Этот файл собирает строго их.
#
# Требования к машине (всё остальное — в репозитории):
#   - Компилятор: MSVC (Visual Studio 2022/2026) на Windows, или gcc/clang на
#     Linux/macOS.
#   - CMake 3.20+.
#   Никаких apt/brew/vcpkg/pkg-config/Python — внешние либы берутся из проекта.
#
# Сборка (Windows, PowerShell — из корня репозитория):
#   cmake -S cmake_schulgin -B build_schulgin -A x64
#   cmake --build build_schulgin --config Release
#   $env:VK_BOT_TOKEN = "<community access token>"
#   $env:VK_GROUP_ID  = "<id сообщества без минуса>"
#   .\build_schulgin\app\main_schulgin.exe
#
# Сборка (Linux/macOS):
#   cmake -S cmake_schulgin -B build_schulgin
#   cmake --build build_schulgin -j
#   export VK_BOT_TOKEN=...  VK_GROUP_ID=...
#   ./build_schulgin/app/main_schulgin
# =============================================================================
cmake_minimum_required(VERSION 3.20)

project(VTL_vkbot_schulgin LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Корень репозитория (этот файл лежит в cmake_schulgin/).
set(VTL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

# exe (и при необходимости .dll рядом) — в build_schulgin/app/.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/app)
foreach(_cfg DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_cfg} ${CMAKE_BINARY_DIR}/app)
endforeach()

# -----------------------------------------------------------------------------
# parson — JSON-парсер, из исходников проекта (external_sources/parson).
# Его CMakeLists делает include dir = external_sources/, поэтому
# `#include <parson/parson.h>` разрешается у всех, кто линкует parson.
# -----------------------------------------------------------------------------
add_subdirectory(${VTL_ROOT}/external_sources/parson
                 ${CMAKE_BINARY_DIR}/parson-build)

# -----------------------------------------------------------------------------
# libcurl — из исходников проекта (external_sources/curl), по платформам:
#   Windows : Schannel (нативный TLS, без OpenSSL)
#   Linux   : OpenSSL из external_libs/openssl (pre-built .a)
#   macOS   : pre-built libcurl.dylib из external_libs/macos
# Логика повторяет cmake/Dependencies-*.cmake, но без FFmpeg/libpq.
# -----------------------------------------------------------------------------
set(BUILD_SHARED_LIBS    OFF CACHE BOOL "" FORCE)
set(BUILD_CURL_EXE       OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING        OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES       OFF CACHE BOOL "" FORCE)
set(BUILD_LIBCURL_DOCS   OFF CACHE BOOL "" FORCE)
set(ENABLE_CURL_MANUAL   OFF CACHE BOOL "" FORCE)
set(CURL_DISABLE_INSTALL ON  CACHE BOOL "" FORCE)
set(CURL_DISABLE_DOCS    ON  CACHE BOOL "" FORCE)
set(CURL_USE_LIBSSH2     OFF CACHE BOOL "" FORCE)
set(CURL_USE_LIBSSH      OFF CACHE BOOL "" FORCE)
set(CURL_USE_LIBPSL      OFF CACHE BOOL "" FORCE)
set(CURL_USE_LIBIDN2     OFF CACHE BOOL "" FORCE)
set(CURL_BROTLI          OFF CACHE BOOL "" FORCE)
set(CURL_ZSTD            OFF CACHE BOOL "" FORCE)
set(CURL_ZLIB            OFF CACHE BOOL "" FORCE)

if(APPLE)
    # macOS — готовый dylib из репозитория, без сборки из исходников.
    set(_macos_curl "${VTL_ROOT}/external_libs/macos/lib/libcurl.dylib")
    if(NOT EXISTS "${_macos_curl}")
        message(FATAL_ERROR "Нет ${_macos_curl} — pre-built libcurl для macOS.")
    endif()
    add_library(CURL::libcurl SHARED IMPORTED GLOBAL)
    set_target_properties(CURL::libcurl PROPERTIES
        IMPORTED_LOCATION "${_macos_curl}"
        INTERFACE_INCLUDE_DIRECTORIES "${VTL_ROOT}/external_libs/macos/include")

elseif(WIN32)
    if(NOT MSVC)
        message(FATAL_ERROR "Под Windows нужен MSVC (Visual Studio). MinGW/MSYS2 не поддержаны.")
    endif()
    # Schannel — нативный TLS Windows, OpenSSL не требуется.
    set(CURL_USE_SCHANNEL ON  CACHE BOOL "" FORCE)
    set(CURL_USE_OPENSSL  OFF CACHE BOOL "" FORCE)
    add_subdirectory(${VTL_ROOT}/external_sources/curl
                     ${CMAKE_BINARY_DIR}/curl-build EXCLUDE_FROM_ALL)
    if(NOT TARGET CURL::libcurl)
        add_library(CURL::libcurl ALIAS libcurl_static)
    endif()

else()
    # Linux — curl из исходников, TLS через OpenSSL из external_libs/openssl.
    set(OPENSSL_ROOT "${VTL_ROOT}/external_libs/openssl")
    if(NOT EXISTS "${OPENSSL_ROOT}/lib/libssl.a")
        message(FATAL_ERROR
            "Нет ${OPENSSL_ROOT}/lib/libssl.a — pre-built OpenSSL для Linux. "
            "Собирается через cmake/scripts/build-openssl-linux.sh (нужен Docker).")
    endif()
    set(OPENSSL_ROOT_DIR        "${OPENSSL_ROOT}" CACHE PATH "" FORCE)
    set(OPENSSL_USE_STATIC_LIBS TRUE CACHE BOOL "" FORCE)
    set(OPENSSL_INCLUDE_DIR     "${OPENSSL_ROOT}/include" CACHE PATH "" FORCE)
    set(OPENSSL_CRYPTO_LIBRARY  "${OPENSSL_ROOT}/lib/libcrypto.a" CACHE FILEPATH "" FORCE)
    set(OPENSSL_SSL_LIBRARY     "${OPENSSL_ROOT}/lib/libssl.a" CACHE FILEPATH "" FORCE)
    find_package(OpenSSL 3.0 REQUIRED)

    set(CURL_USE_OPENSSL  ON  CACHE BOOL "" FORCE)
    set(CURL_DISABLE_LDAP  ON CACHE BOOL "" FORCE)
    set(CURL_DISABLE_LDAPS ON CACHE BOOL "" FORCE)
    add_subdirectory(${VTL_ROOT}/external_sources/curl
                     ${CMAKE_BINARY_DIR}/curl-build EXCLUDE_FROM_ALL)
endif()

# -----------------------------------------------------------------------------
# VTL_vkbot — статическая библиотека модуля + HTTP-клиент.
# (HTTP-клиент компилируем сюда же — в корневом проекте он приходит из VTL_utils.)
# -----------------------------------------------------------------------------
add_library(VTL_vkbot STATIC
    ${VTL_ROOT}/VTL/vkbot/VTL_vkbot.c
    ${VTL_ROOT}/VTL/vkbot/api/VTL_vkbot_api.c
    ${VTL_ROOT}/VTL/vkbot/store/VTL_vkbot_store.c
    ${VTL_ROOT}/VTL/vkbot/command/VTL_vkbot_command.c
    ${VTL_ROOT}/VTL/utils/curl/VTL_utils_curl_http_client.c
)
target_include_directories(VTL_vkbot PUBLIC ${VTL_ROOT})
target_link_libraries(VTL_vkbot PUBLIC parson CURL::libcurl)
set_target_properties(VTL_vkbot PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)

if(MSVC)
    target_compile_options(VTL_vkbot PRIVATE /W4 /utf-8)
else()
    target_compile_options(VTL_vkbot PRIVATE
        -Wall -Wextra
        -Wno-error=implicit-function-declaration
        -Wno-error=incompatible-pointer-types
        -Wno-error=int-conversion
        -Wno-unused-parameter)
endif()

# -----------------------------------------------------------------------------
# main_schulgin — исполняемый файл VK-бота.
# -----------------------------------------------------------------------------
add_executable(main_schulgin ${VTL_ROOT}/main_schulgin.c)
target_link_libraries(main_schulgin PRIVATE VTL_vkbot)

find_package(Threads QUIET)
if(Threads_FOUND)
    target_link_libraries(main_schulgin PRIVATE Threads::Threads)
endif()
