# VTL_vkbot — модуль VK-бота (личные финансы). Зависит только от parson (JSON)
# и libcurl (HTTP).

add_library(VTL_vkbot STATIC
    VTL_vkbot.c
    api/VTL_vkbot_api.c
    store/VTL_vkbot_store.c
    command/VTL_vkbot_command.c
)

target_include_directories(VTL_vkbot PUBLIC ${CMAKE_SOURCE_DIR})

# Символы HTTP-клиента (VTL_curl_http_client_*) здесь НЕ компилируются: в общий
# exe они придут из VTL_utils, а в main_schulgin докомпилируются отдельно.
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

# Тест store-логики. Использует мини-фреймворк из VTL/test. Регистрация в ctest —
# в корневом CMakeLists, после enable_testing().
add_executable(vtl_vkbot_test
    test/VTL_vkbot_store_test.c
    ${CMAKE_SOURCE_DIR}/VTL/test/VTL_test_data.c
)
target_link_libraries(vtl_vkbot_test PRIVATE VTL_vkbot)
target_include_directories(vtl_vkbot_test PRIVATE ${CMAKE_SOURCE_DIR})
if(MSVC)
    target_compile_options(vtl_vkbot_test PRIVATE /W4 /utf-8)
else()
    target_compile_options(vtl_vkbot_test PRIVATE -Wall -Wextra)
endif()
