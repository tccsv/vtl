# Модуль Telegram-бота.

add_library(VTL_tgbot STATIC
    VTL_tgbot.c
    api/VTL_tgbot_api.c
    session/VTL_tgbot_session.c
    command/VTL_tgbot_command.c
)

target_include_directories(VTL_tgbot PUBLIC ${CMAKE_SOURCE_DIR})

target_link_libraries(VTL_tgbot
    PUBLIC
        parson
        CURL::libcurl
)

set_target_properties(VTL_tgbot PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

if(MSVC)
    target_compile_options(VTL_tgbot PRIVATE /W4 /utf-8)
else()
    target_compile_options(VTL_tgbot PRIVATE
        -Wall -Wextra
        -Wno-error=implicit-function-declaration
        -Wno-error=incompatible-pointer-types
        -Wno-error=int-conversion
        -Wno-unused-parameter
    )
endif()

# Тест логики сессии.
add_executable(VTL_tgbot_test
    test/VTL_tgbot_session_test.c
    ${CMAKE_SOURCE_DIR}/VTL/test/VTL_test_data.c
)
target_link_libraries(VTL_tgbot_test PRIVATE VTL_tgbot)
target_include_directories(VTL_tgbot_test PRIVATE ${CMAKE_SOURCE_DIR})
if(MSVC)
    target_compile_options(VTL_tgbot_test PRIVATE /W4 /utf-8)
else()
    target_compile_options(VTL_tgbot_test PRIVATE -Wall -Wextra)
endif()
