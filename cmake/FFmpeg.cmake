include(ExternalProject)

# Настройки для FFmpeg
set(FFMPEG_VERSION "n7.0.2")
set(FFMPEG_SOURCE_DIR "${CMAKE_BINARY_DIR}/ffmpeg-src")
set(FFMPEG_BUILD_DIR "${CMAKE_BINARY_DIR}/ffmpeg-build")
set(FFMPEG_INSTALL_DIR "${CMAKE_BINARY_DIR}/ffmpeg-install")

# Находим системные библиотеки FFmpeg
find_library(AVCODEC_LIBRARY avcodec)
find_library(AVFORMAT_LIBRARY avformat)
find_library(AVUTIL_LIBRARY avutil)
find_library(SWSCALE_LIBRARY swscale)
find_library(AVFILTER_LIBRARY avfilter)

# Проверяем, что все библиотеки найдены
if(NOT AVCODEC_LIBRARY OR NOT AVFORMAT_LIBRARY OR NOT AVUTIL_LIBRARY OR NOT SWSCALE_LIBRARY OR NOT AVFILTER_LIBRARY)
    message(FATAL_ERROR "Не удалось найти необходимые библиотеки FFmpeg. Установите ffmpeg: sudo pacman -S ffmpeg")
endif()

# Создаем импортированные библиотеки
add_library(avcodec SHARED IMPORTED)
add_library(avformat SHARED IMPORTED)
add_library(avutil SHARED IMPORTED)
add_library(swscale SHARED IMPORTED)
add_library(avfilter SHARED IMPORTED)

# Устанавливаем свойства импортированных библиотек
set_target_properties(avcodec PROPERTIES
    IMPORTED_LOCATION ${AVCODEC_LIBRARY}
    INTERFACE_INCLUDE_DIRECTORIES /usr/include
)

set_target_properties(avformat PROPERTIES
    IMPORTED_LOCATION ${AVFORMAT_LIBRARY}
    INTERFACE_INCLUDE_DIRECTORIES /usr/include
)

set_target_properties(avutil PROPERTIES
    IMPORTED_LOCATION ${AVUTIL_LIBRARY}
    INTERFACE_INCLUDE_DIRECTORIES /usr/include
)

set_target_properties(swscale PROPERTIES
    IMPORTED_LOCATION ${SWSCALE_LIBRARY}
    INTERFACE_INCLUDE_DIRECTORIES /usr/include
)

set_target_properties(avfilter PROPERTIES
    IMPORTED_LOCATION ${AVFILTER_LIBRARY}
    INTERFACE_INCLUDE_DIRECTORIES /usr/include
)

# Создаем интерфейсную библиотеку для FFmpeg
add_library(ffmpeg_libs INTERFACE)
target_link_libraries(ffmpeg_libs INTERFACE
    avcodec
    avformat
    avutil
    swscale
    avfilter
) 