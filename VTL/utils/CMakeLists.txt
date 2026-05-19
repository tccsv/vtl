add_library(VTL_utils OBJECT
    VTL_time.c
    VTL_file.c
    VTL_encryptor.c
    log/VTL_console_log.c
    curl/VTL_http_client.c
    http_client.c
    db/VTL_db_credentals.c
    db/VTL_db_connection.c
    db/VTL_db_executor.c
)

target_include_directories(VTL_utils PUBLIC ${CMAKE_SOURCE_DIR})

target_link_libraries(VTL_utils
    PUBLIC
        parson
        CURL::libcurl
        PostgreSQL::PostgreSQL
)

if(UNIX)
    target_link_libraries(VTL_utils PUBLIC m)
endif()

set_target_properties(VTL_utils PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

if(MSVC)
    target_compile_options(VTL_utils PRIVATE /W4 /utf-8)
else()
    # GCC 14+ по дефолту делает следующие диагностики ошибками; на GCC 13 и старше
    # они были варнингами, и проект собирался. Возвращаем им статус warning,
    # чтобы исходники собирались на современных тулчейнах. Сами варнинги остаются
    # видимыми — это не -Wno-..., это -Wno-error=...
    target_compile_options(VTL_utils PRIVATE
        -Wall -Wextra
        -Wno-error=implicit-function-declaration
        -Wno-error=incompatible-pointer-types
        -Wno-error=int-conversion
    )
endif()
