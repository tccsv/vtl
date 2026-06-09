# VTL_bot — модуль Telegram-бота (пульт публикации VTL). Сама библиотека лёгкая:
# зависит только от parson (JSON) и libcurl (HTTP). Реальные функции публикации
# проекта прокидываются снаружи через VTL_bot_Handlers (см. main_firyulin.c),
# поэтому весь VTL сюда не тянется и юнит-тест собирается в изоляции.

add_library(VTL_bot STATIC
    VTL_bot.c
    api/VTL_bot_api.c
    session/VTL_bot_session.c
    command/VTL_bot_command.c
)

target_include_directories(VTL_bot PUBLIC ${CMAKE_SOURCE_DIR})

# Символы HTTP-клиента (VTL_curl_http_client_*) здесь НЕ компилируются: в общий
# exe они придут из VTL_utils, а в main_firyulin докомпилируются отдельно.
target_link_libraries(VTL_bot
    PUBLIC
        parson
        CURL::libcurl
)

set_target_properties(VTL_bot PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

if(MSVC)
    target_compile_options(VTL_bot PRIVATE /W4 /utf-8)
else()
    target_compile_options(VTL_bot PRIVATE
        -Wall -Wextra
        -Wno-error=implicit-function-declaration
        -Wno-error=incompatible-pointer-types
        -Wno-error=int-conversion
        -Wno-unused-parameter
    )
endif()

# Тест логики сессии (выбор платформ/формата). Использует мини-фреймворк из
# VTL/test. Регистрация в ctest — в корневом CMakeLists, после enable_testing().
add_executable(vtl_bot_test
    test/VTL_bot_session_test.c
    ${CMAKE_SOURCE_DIR}/VTL/test/VTL_test_data.c
)
target_link_libraries(vtl_bot_test PRIVATE VTL_bot)
target_include_directories(vtl_bot_test PRIVATE ${CMAKE_SOURCE_DIR})
if(MSVC)
    target_compile_options(vtl_bot_test PRIVATE /W4 /utf-8)
else()
    target_compile_options(vtl_bot_test PRIVATE -Wall -Wextra)
endif()
