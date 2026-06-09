# VTL_vkbot — модуль бота сообщества ВКонтакте. Лёгкий: parson (JSON) + libcurl
# (HTTP). Функции публикации проекта прокидываются снаружи через VTL_vk_Publishers
# (см. main_schulgin.c) — весь VTL сюда не тянется, тест собирается автономно.

add_library(VTL_vkbot STATIC
    VTL_vk_bot.c
    VTL_vk_api.c
    VTL_vk_dialog.c
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

# Тест разбора/состояния. Регистрация в ctest — в корневом CMakeLists.
add_executable(vtl_vkbot_test
    test/VTL_vk_dialog_test.c
    ${CMAKE_SOURCE_DIR}/VTL/test/VTL_test_data.c
)
target_link_libraries(vtl_vkbot_test PRIVATE VTL_vkbot)
target_include_directories(vtl_vkbot_test PRIVATE ${CMAKE_SOURCE_DIR})
if(MSVC)
    target_compile_options(vtl_vkbot_test PRIVATE /W4 /utf-8)
else()
    target_compile_options(vtl_vkbot_test PRIVATE -Wall -Wextra)
endif()
