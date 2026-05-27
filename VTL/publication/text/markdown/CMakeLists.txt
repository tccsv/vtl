# Самодостаточная под-библиотека markdown-модуля.
# Собирается отдельно от основного проекта VTL — не тянет FFmpeg/curl/libpq.
# Структура и принципы — те же, что у telegram/CMakeLists.txt.
#
# Поддерживаемые компиляторы: MSVC, GCC (incl. MinGW-w64), Clang.
# Threading: pthread на Linux/macOS, WinAPI на Windows — header markdown_threads.h.
#
# Использование изолированно:
#   cd VTL/publication/text/markdown
#   cmake -S . -B build
#   cmake --build build
#   ./build/vtl_markdown_test          (Linux/macOS)
#   .\build\vtl_markdown_test.exe      (Windows)

cmake_minimum_required(VERSION 3.20)
project(VTL_markdown LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Корень репо — нужен для include путей вида <VTL/publication/...>
get_filename_component(VTL_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.."
                       ABSOLUTE)

# ============================================================
# Static-библиотека VTL_markdown
# ============================================================
add_library(VTL_markdown STATIC
    VTL_publication_text_op_markdown.c
    VTL_publication_text_op_markdown_data.c
    VTL_publication_text_op_markdown_scan.c
    VTL_publication_text_op_markdown_convert.c
    # типы VTL_publication_Text / MarkedText из data.c (без внешних зависимостей)
    ../VTL_publication_text_data.c
)

target_include_directories(VTL_markdown PUBLIC ${VTL_REPO_ROOT})

# pthread на UNIX. Под Windows ничего не нужно — WinAPI всегда доступен.
if(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(VTL_markdown PUBLIC Threads::Threads)
endif()

if(MSVC)
    target_compile_options(VTL_markdown PRIVATE /W4 /utf-8)
else()
    target_compile_options(VTL_markdown PRIVATE
        -Wall -Wextra
        # тот же pre-existing баг с макросами в data.h — у нас обойдён локально
        -Wno-shift-negative-value
    )
endif()

# ============================================================
# Тестовая программа: проверка корректности + бенчмарк
# ============================================================
add_executable(vtl_markdown_test test/VTL_publication_text_op_markdown_test.c)
target_link_libraries(vtl_markdown_test PRIVATE VTL_markdown)

if(MSVC)
    target_compile_options(vtl_markdown_test PRIVATE /W4 /utf-8)
else()
    target_compile_options(vtl_markdown_test PRIVATE -Wall -Wextra)
endif()
