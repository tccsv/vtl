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
add_subdirectory(VTL/bots/tgbot)
add_subdirectory(VTL/bots/vkbot)

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

# Telegram-бот: cmake --build build --target main_firyulin
add_executable(main_firyulin EXCLUDE_FROM_ALL main_firyulin.c)
target_link_libraries(main_firyulin PRIVATE
    VTL_tgbot
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
    target_link_libraries(main_firyulin PRIVATE dl)
endif()

# Демо TG-бота без FFmpeg (заглушки публикации). Историю/шифратор и http-клиент
# докомпилируем напрямую, чтобы не тянуть VTL_utils с libpq/FFmpeg.
add_executable(main_firyulin_demo EXCLUDE_FROM_ALL
    tools/bots/tgbot/main_firyulin_demo.c
    VTL/utils/curl/VTL_utils_curl_http_client.c
    VTL/user/history/db/VTL_user_history_db_save.c
    VTL/user/history/VTL_user_history_data.c
    VTL/user/VTL_user_data.c
    VTL/utils/VTL_utils_encryptor.c
)
target_link_libraries(main_firyulin_demo PRIVATE VTL_tgbot Threads::Threads)

# VK-бот: cmake --build build --target main_schulgin
add_executable(main_schulgin EXCLUDE_FROM_ALL main_schulgin.c)
target_link_libraries(main_schulgin PRIVATE
    VTL_vkbot
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
    target_link_libraries(main_schulgin PRIVATE dl)
endif()

# Демо VK-бота без FFmpeg (заглушки). История/шифратор и http-клиент
# докомпилируются напрямую, без libpq/FFmpeg.
#   cmake --build build --target main_schulgin_demo
add_executable(main_schulgin_demo EXCLUDE_FROM_ALL
    tools/bots/vkbot/main_schulgin_demo.c
    VTL/utils/curl/VTL_utils_curl_http_client.c
    VTL/user/history/db/VTL_user_history_db_save.c
    VTL/user/history/VTL_user_history_data.c
    VTL/user/VTL_user_data.c
    VTL/utils/VTL_utils_encryptor.c
)
target_link_libraries(main_schulgin_demo PRIVATE VTL_vkbot Threads::Threads)

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

# Юнит-тест бота (цель VTL_tgbot_test собирается в VTL/bots/tgbot/CMakeLists).
add_test(NAME VTL_tgbot_session_test COMMAND VTL_tgbot_test)

# Юнит-тест VK-бота (цель vtl_vkbot_test в VTL/bots/vkbot/CMakeLists).
add_test(NAME VTL_vk_session_test COMMAND vtl_vkbot_test)

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
