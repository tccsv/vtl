cmake_minimum_required(VERSION 3.20)

project(VTL LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(AppName ${CMAKE_PROJECT_NAME})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/app)
# Под Visual Studio (multi-config generator) по умолчанию exe попадает
# в app/Debug или app/Release. DLL лежат рядом, в app/ — Windows их там
# и ищет. Принудительно кладём exe в app/ независимо от конфига.
foreach(_cfg DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_cfg} ${CMAKE_SOURCE_DIR}/app)
endforeach()


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
