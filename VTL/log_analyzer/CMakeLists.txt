cmake_minimum_required(VERSION 3.16)
project(VTL_logAnalyzer C)

set(CMAKE_C_STANDARD 11)

# ------------------------------------------------------------------ #
#  VTL_logAnalyzer — параллельный анализатор логов пользователя VTL  #
#                                                                      #
#  Запуск:                                                             #
#    ./VTL_logAnalyzer <файл.jsonl> [n_threads]                       #
#    ./VTL_logAnalyzer <файл.jsonl> --demo [n_threads]                #
# ------------------------------------------------------------------ #

add_executable(VTL_logAnalyzer
        VTL_logAnalyzer_parse.c
        VTL_logAnalyzer_search.c
        VTL_logAnalyzer_stats.c
        VTL_logAnalyzer_demo.c
)

# для include вида <VTL/log_analyzer/...> нужна папка содержащая VTL/
# CMAKE_SOURCE_DIR = .../log_analyzer, поэтому поднимаемся на два уровня
target_include_directories(VTL_logAnalyzer PRIVATE
        ${CMAKE_SOURCE_DIR}/../..
)

target_include_directories(VTL_logAnalyzer PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/VTL"
)


# на Linux нужен pthread, на Windows WinAPI встроен
if (UNIX)
    target_link_libraries(VTL_logAnalyzer PRIVATE pthread m)
endif()

# GCC/Clang флаги кодировки — не нужны на MSVC
if (NOT MSVC)
    target_compile_options(VTL_logAnalyzer PRIVATE
            -finput-charset=UTF-8
            -fexec-charset=UTF-8
    )
endif()
