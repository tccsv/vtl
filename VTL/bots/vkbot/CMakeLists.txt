# Бот сообщества ВКонтакте: parson + libcurl.

add_library(VTL_vkbot STATIC
    VTL_vk_bot.c
    api/VTL_vk_api.c
    session/VTL_vk_session.c
    command/VTL_vk_command.c
)

target_include_directories(VTL_vkbot PUBLIC ${CMAKE_SOURCE_DIR})

target_link_libraries(VTL_vkbot
    PUBLIC
        parson
        CURL::libcurl
)

set_target_properties(VTL_vkbot PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

if(MSVC)
    target_compile_options(VTL_vkbot PRIVATE /W4 /utf-8)
else()
    target_compile_options(VTL_vkbot PRIVATE
        -Wall -Wextra
        -Wno-error=implicit-function-declaration
        -Wno-error=incompatible-pointer-types
        -Wno-error=int-conversion
        -Wno-unused-parameter
    )
endif()

# Тест разбора/состояния.
add_executable(vtl_vkbot_test
    test/VTL_vk_session_test.c
    ${CMAKE_SOURCE_DIR}/VTL/test/VTL_test_data.c
    ${CMAKE_SOURCE_DIR}/VTL/user/history/db/VTL_user_history_db_save.c
    ${CMAKE_SOURCE_DIR}/VTL/user/history/VTL_user_history_data.c
    ${CMAKE_SOURCE_DIR}/VTL/user/VTL_user_data.c
    ${CMAKE_SOURCE_DIR}/VTL/utils/VTL_utils_encryptor.c
)
target_link_libraries(vtl_vkbot_test PRIVATE VTL_vkbot)
target_include_directories(vtl_vkbot_test PRIVATE ${CMAKE_SOURCE_DIR})
if(MSVC)
    target_compile_options(vtl_vkbot_test PRIVATE /W4 /utf-8)
else()
    target_compile_options(vtl_vkbot_test PRIVATE -Wall -Wextra)
endif()
