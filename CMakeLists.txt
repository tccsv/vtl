cmake_minimum_required(VERSION 3.20)

project(VTL LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(AppName ${CMAKE_PROJECT_NAME})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/app)


# ============================================================
# Режимы сборки
# ============================================================
# По умолчанию (VTL_MINIMAL_BUILD=OFF) собирается полный проект:
#   Telegram + Reddit + аудио/видео через FFmpeg + PostgreSQL.
#   Требует FFmpeg-runtime-deps (libvpx, libopus, libdav1d и т.п.) — есть на
#   обычной desktop-Linux с медиаподдержкой, нет на голом server-Ubuntu.
#
# VTL_MINIMAL_BUILD=ON — упрощённая сборка без FFmpeg/curl/openssl/postgres.
# Работает на абсолютно чистой Linux без единой установленной библиотеки
# кроме toolchain (build-essential + cmake).
option(VTL_MINIMAL_BUILD "Упрощённая сборка без внешних зависимостей" OFF)


if(VTL_MINIMAL_BUILD)
    message(STATUS "VTL: minimal build")
    message(STATUS "VTL: target system = ${CMAKE_SYSTEM_NAME}")

    add_subdirectory(VTL/publication/text/asciidoc)

    add_executable(${AppName} main.c)
    target_compile_definitions(${AppName} PRIVATE VTL_MINIMAL_BUILD)
    target_link_libraries(${AppName} PRIVATE VTL_asciidoc)
    # Threads уже линкуются через VTL_asciidoc (Threads::Threads).
else()
    message(STATUS "VTL: FULL build (AsciiDoc + Telegram/Reddit/FFmpeg/Postgres)")

    # Все внешние библиотеки (FFmpeg, libcurl, OpenSSL, libpq) лежат в external_libs/
    # и подключаются как IMPORTED-таргеты — никаких apt/brew/vcpkg/pkg-config не требуется.
    include(cmake/Dependencies.cmake)

    add_subdirectory(external_sources/parson)

    add_subdirectory(VTL/utils)
    add_subdirectory(VTL/user)
    add_subdirectory(VTL/publication)
    add_subdirectory(VTL/media_container)
    add_subdirectory(VTL/content_platform)

    find_package(Threads REQUIRED)

    add_executable(${AppName} main.c)

    target_link_libraries(${AppName} PRIVATE
        VTL_content_platform
        VTL_publication
        VTL_media_container
        VTL_user
        VTL_utils
        Threads::Threads
    )

    if(UNIX)
        target_link_libraries(${AppName} PRIVATE dl)
    endif()

    # MinGW: clock_gettime/pthread живут в нашем libwinpthread из external_libs
    if(MINGW)
        target_link_libraries(${AppName} PRIVATE VTL::winpthread)
    endif()
endif()

# Apple Silicon strict-проверка подписи — без ad-hoc codesign бинарь
# падает с 'zsh: killed' при запуске.
if(APPLE)
    find_program(VTL_CODESIGN codesign REQUIRED)
    add_custom_command(TARGET ${AppName} POST_BUILD
        COMMAND "${VTL_CODESIGN}" --force --sign - "$<TARGET_FILE:${AppName}>"
        VERBATIM
    )
endif()

enable_testing()
add_subdirectory(VTL/test)

add_test(
    NAME VTL_mediawiki_pipeline
    COMMAND ${AppName} --mediawiki ${CMAKE_SOURCE_DIR}/text.wiki
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
set_tests_properties(VTL_mediawiki_pipeline PROPERTIES
    PASS_REGULAR_EXPRESSION "Text Pipeline \\(MediaWiki"
)
