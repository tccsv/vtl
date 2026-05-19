# Самодостаточная под-библиотека модуля publication.
# Не зависит ни от utils, ни от curl/openssl/postgres/ffmpeg.
# Работает на Linux, macOS, Windows (MSVC) — threading через собственный
# wrapper (WinAPI на Windows, pthread на остальных).

add_library(VTL_asciidoc STATIC
    VTL_publication_text_op_asciidoc.c
    VTL_publication_text_op_asciidoc_data.c
    VTL_publication_text_op_asciidoc_scan.c
    VTL_publication_text_op_asciidoc_convert.c
    infra/VTL_publication_text_op_asciidoc_load.c
    # Парсер использует типы VTL_publication_Text / MarkedText
    # из publication/text/VTL_publication_text_data.h — поэтому подключаем
    # data.c (структуры без внешних зависимостей).
    ../VTL_publication_text_data.c
)

target_include_directories(VTL_asciidoc PUBLIC ${CMAKE_SOURCE_DIR})

# Threads — нужны только под UNIX (-lpthread). Под Windows compat.h уходит
# в WinAPI-ветку (CreateThread/WaitForSingleObject), pthread-линковка не нужна.
if(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(VTL_asciidoc PUBLIC Threads::Threads)
endif()

set_target_properties(VTL_asciidoc PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

if(MSVC)
    target_compile_options(VTL_asciidoc PRIVATE /W4 /utf-8)
else()
    target_compile_options(VTL_asciidoc PRIVATE
        -Wall -Wextra
        -Wno-error=implicit-function-declaration
        -Wno-error=incompatible-pointer-types
        -Wno-error=int-conversion
        -Wno-unused-parameter
    )
endif()
