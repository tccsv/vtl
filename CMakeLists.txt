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
add_subdirectory(external_sources/ed25519)

add_subdirectory(VTL/utils/threading)
add_subdirectory(VTL/utils/auth)
add_subdirectory(VTL/scheduler)
add_subdirectory(VTL/utils)
add_subdirectory(VTL/user)
add_subdirectory(VTL/publication)
add_subdirectory(VTL/media_container)
add_subdirectory(VTL/media_container/encoding)
add_subdirectory(VTL/content_platform)
add_subdirectory(VTL/history_administration)
add_subdirectory(VTL/bot)

find_package(Threads REQUIRED)

add_executable(${AppName} main.c)

target_link_libraries(${AppName} PRIVATE
    VTL_scheduler
    VTL_content_platform
    VTL_publication
    VTL_media_container
    VTL_media_container_encoding
    VTL_history_administration
    VTL_user
    VTL_utils
    VTL_utils_auth
    Threads::Threads
)

if(UNIX)
    target_link_libraries(${AppName} PRIVATE dl)
endif()

# main_timofeenko — standalone-демо моей зоны (AsciiDoc-парсер).
# Линкуется только со standalone target VTL_asciidoc (см. одноимённый
# CMakeLists в VTL/publication/text/asciidoc/) — никакого FFmpeg/curl/libpq.
# EXCLUDE_FROM_ALL: target не собирается с дефолтным `cmake --build`,
# только когда явно тянется через main_timofeenko.
add_subdirectory(VTL/publication/text/asciidoc
                 ${CMAKE_BINARY_DIR}/VTL_asciidoc_standalone
                 EXCLUDE_FROM_ALL)
add_executable(main_timofeenko main_timofeenko.c)
target_link_libraries(main_timofeenko PRIVATE VTL_asciidoc)

# Standalone-exe бота. HTTP-клиент докомпилируем сюда напрямую, чтобы не тянуть
# весь VTL_utils. EXCLUDE_FROM_ALL — собирается явной целью:
#   cmake --build build --target main_firyulin
add_executable(main_firyulin EXCLUDE_FROM_ALL
    main_firyulin.c
    VTL/utils/curl/VTL_utils_curl_http_client.c
)
target_link_libraries(main_firyulin PRIVATE VTL_bot)

# Apple Silicon strict-проверка подписи — без ad-hoc codesign бинарь
# падает с 'zsh: killed' при запуске.
if(APPLE)
    find_program(VTL_CODESIGN codesign REQUIRED)
    add_custom_command(TARGET ${AppName} POST_BUILD
        COMMAND "${VTL_CODESIGN}" --force --sign - "$<TARGET_FILE:${AppName}>"
        VERBATIM
    )
endif()

# На Linux FFmpeg линкуется статически (см. Dependencies-Linux.cmake:
# --enable-static --disable-shared). Всё внутри VTL exe — нет .so рядом,
# копировать нечего. libpq остаётся SHARED (его .so target ставится в app/
# через LIBRARY_OUTPUT_DIRECTORY автоматом).

enable_testing()
add_subdirectory(VTL/test)

# Юнит-тест бота (цель vtl_bot_test собирается в VTL/bot/CMakeLists).
add_test(NAME VTL_bot_store_test COMMAND vtl_bot_test)

add_test(
    NAME VTL_mediawiki_pipeline
    COMMAND ${AppName} --mediawiki ${CMAKE_SOURCE_DIR}/text.wiki
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
set_tests_properties(VTL_mediawiki_pipeline PROPERTIES
    PASS_REGULAR_EXPRESSION "Text Pipeline \\(MediaWiki"
)

add_test(
    NAME VTL_asciidoc_pipeline
    COMMAND ${AppName} --asciidoc ${CMAKE_SOURCE_DIR}/text.adoc
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
set_tests_properties(VTL_asciidoc_pipeline PROPERTIES
    PASS_REGULAR_EXPRESSION "Text Pipeline \\(AsciiDoc"
)
