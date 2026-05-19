# Самодостаточная под-библиотека telegram-модуля.
# Собирается отдельно от основного проекта VTL — не тянет FFmpeg/curl/libpq.
# Удобно для отладки и быстрых итераций.
#
# Поддерживаемые компиляторы:
#   - MSVC (Visual Studio 2019+)
#   - GCC (Linux, macOS, MinGW-w64 на Windows)
#   - Clang (Linux, macOS, clang-cl на Windows)
#
# Threading — через собственный header (telegram_threads.h):
# pthread на Linux/macOS, WinAPI на Windows. Линкуется автоматически.
#
# Использование изолированно:
#   cd VTL/publication/text/telegram
#   cmake -S . -B build
#   cmake --build build
#   ./build/vtl_telegram_test          (Linux/macOS)
#   .\build\vtl_telegram_test.exe      (Windows)

cmake_minimum_required(VERSION 3.20)
project(VTL_telegram LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Корень репо — нужен для include путей вида <VTL/publication/...>
get_filename_component(VTL_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.."
                       ABSOLUTE)

# ============================================================
# Static-библиотека VTL_telegram
# ============================================================
add_library(VTL_telegram STATIC
    VTL_publication_text_op_telegram.c
    VTL_publication_text_op_telegram_data.c
    VTL_publication_text_op_telegram_scan.c
    VTL_publication_text_op_telegram_convert.c
    # типы VTL_publication_Text / MarkedText из data.c (без внешних зависимостей)
    ../VTL_publication_text_data.c
)

target_include_directories(VTL_telegram PUBLIC ${VTL_REPO_ROOT})

# pthread на UNIX (Linux/macOS). На Windows ничего не нужно — WinAPI всегда доступен.
if(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(VTL_telegram PUBLIC Threads::Threads)
endif()

if(MSVC)
    # /utf-8 — кириллица в комментариях/строках корректно декодируется компилятором
    target_compile_options(VTL_telegram PRIVATE /W4 /utf-8)
else()
    target_compile_options(VTL_telegram PRIVATE
        -Wall -Wextra
        # тот самый pre-existing баг с макросами в data.h — у нас обойдён локально,
        # но при компиляции data.h gcc всё равно ругается. Глушим только этот варнинг.
        -Wno-shift-negative-value
    )
endif()

# ============================================================
# Тестовая программа: проверка корректности + бенчмарк
# ============================================================
add_executable(vtl_telegram_test test/test_telegram.c)
target_link_libraries(vtl_telegram_test PRIVATE VTL_telegram)

if(MSVC)
    target_compile_options(vtl_telegram_test PRIVATE /W4 /utf-8)
else()
    target_compile_options(vtl_telegram_test PRIVATE -Wall -Wextra)
endif()
