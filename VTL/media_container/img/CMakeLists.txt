# Находим необходимые пакеты
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavfilter
    libavutil
    libswscale
)

# Создаем библиотеку для обработки изображений
add_library(VTL_img STATIC
    VTL_img_core.c
    VTL_img_filters.c
    VTL_img_utils.c
)

# Устанавливаем свойства библиотеки
set_target_properties(VTL_img PROPERTIES
    OUTPUT_NAME "VTL_img"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Добавляем заголовочные файлы
target_include_directories(VTL_img
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${FFMPEG_INCLUDE_DIRS}
)

# Линкуем с FFmpeg
target_link_libraries(VTL_img
    PRIVATE
        PkgConfig::FFMPEG
)

# Устанавливаем флаги компиляции
target_compile_options(VTL_img PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wno-unused-parameter
)

# Устанавливаем определения
target_compile_definitions(VTL_img PRIVATE
    _GNU_SOURCE
    VTL_IMG_EXPORTS
)

# Устанавливаем стандарт C
set_target_properties(VTL_img PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

# Устанавливаем выходную директорию
set_target_properties(VTL_img PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# Устанавливаем зависимости для установки
install(TARGETS VTL_img
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# Устанавливаем заголовочные файлы
install(FILES
    VTL_img.h
    VTL_img_core.h
    VTL_img_filters.h
    VTL_img_utils.h
    VTL_img_data.h
    DESTINATION include/VTL/img
) 