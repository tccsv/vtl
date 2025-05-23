include(FetchContent)
include(ExternalProject)

set(FFMPEG_BUILD_SHARED_LIBS ON CACHE BOOL "Build FFmpeg as shared libraries")
set(FFMPEG_ENABLE_GPL ON CACHE BOOL "Enable GPL code")
set(FFMPEG_ENABLE_NONFREE ON CACHE BOOL "Enable nonfree code")

set(FFMPEG_INSTALL_DIR ${CMAKE_BINARY_DIR}/ffmpeg_install)

# Создаем файл с хешем конфигурации
set(FFMPEG_CONFIG_HASH_FILE ${CMAKE_BINARY_DIR}/ffmpeg_config_hash.txt)
set(FFMPEG_CONFIGURE_OPTIONS
    --enable-static
    --disable-shared
    --enable-pic
    --disable-doc
    --disable-programs
    --enable-avdevice
    --enable-swscale
    --enable-avfilter
    --enable-network
    --enable-protocol=file
    --enable-decoder=h264
    --enable-decoder=hevc
    --enable-decoder=mpeg4
    --enable-demuxer=mov
    --enable-demuxer=matroska
    --enable-encoder=png
    --enable-encoder=jpeg
    --enable-encoder=mjpeg
    --enable-muxer=image2
    --enable-muxer=matroska
    --enable-filter=scale
    --enable-filter=crop
    --enable-filter=rotate
    --enable-filter=transpose
    --enable-filter=format
    --enable-filter=colorchannelmixer
    --enable-filter=convolution
    --enable-filter=edgedetect
    --enable-filter=boxblur
    --enable-filter=gblur
    --enable-filter=unsharp
    --enable-filter=hue
    --enable-filter=transpose
    --enable-swscale
    --enable-swscale-alpha
    --prefix=${FFMPEG_INSTALL_DIR}
)

if(FFMPEG_ENABLE_GPL)
    list(APPEND FFMPEG_CONFIGURE_OPTIONS --enable-gpl)
endif()

if(FFMPEG_ENABLE_NONFREE)
    list(APPEND FFMPEG_CONFIGURE_OPTIONS --enable-nonfree)
endif()

# Создаем команду для генерации хеша конфигурации
set(FFMPEG_CONFIG_HASH_CMD
    COMMAND ${CMAKE_COMMAND} -E echo "${FFMPEG_CONFIGURE_OPTIONS}" | md5sum > ${FFMPEG_CONFIG_HASH_FILE}
)

# Проверяем, нужно ли пересобирать FFmpeg
set(FFMPEG_NEEDS_REBUILD FALSE)
if(EXISTS ${FFMPEG_CONFIG_HASH_FILE})
    file(READ ${FFMPEG_CONFIG_HASH_FILE} OLD_HASH)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E echo "${FFMPEG_CONFIGURE_OPTIONS}" | md5sum
        OUTPUT_VARIABLE NEW_HASH
    )
    if(NOT "${OLD_HASH}" STREQUAL "${NEW_HASH}")
        set(FFMPEG_NEEDS_REBUILD TRUE)
    endif()
else()
    set(FFMPEG_NEEDS_REBUILD TRUE)
endif()

if(FFMPEG_NEEDS_REBUILD)
    ExternalProject_Add(ffmpeg_ext
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/ffmpeg
        CONFIGURE_COMMAND ${FFMPEG_CONFIG_HASH_CMD}
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/external/ffmpeg/configure ${FFMPEG_CONFIGURE_OPTIONS}
        BUILD_COMMAND make -j${CMAKE_BUILD_PARALLEL_LEVEL}
        INSTALL_COMMAND make install
        BUILD_IN_SOURCE 1
    )
else()
    # Если пересборка не нужна, просто создаем пустую цель
    add_custom_target(ffmpeg_ext)
endif()

# Set FFmpeg include directories
set(FFMPEG_INCLUDE_DIRS
    ${FFMPEG_INSTALL_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/ffmpeg
    ${CMAKE_CURRENT_SOURCE_DIR}/external/ffmpeg/libavcodec
    ${CMAKE_CURRENT_SOURCE_DIR}/external/ffmpeg/libavformat
    ${CMAKE_CURRENT_SOURCE_DIR}/external/ffmpeg/libavutil
    ${CMAKE_CURRENT_SOURCE_DIR}/external/ffmpeg/libswresample
    ${CMAKE_CURRENT_SOURCE_DIR}/external/ffmpeg/libswscale
    ${CMAKE_CURRENT_SOURCE_DIR}/external/ffmpeg/libavfilter
    ${CMAKE_CURRENT_SOURCE_DIR}/external/ffmpeg/libavdevice
)

# Set FFmpeg libraries
set(FFMPEG_LIBRARIES
    ${FFMPEG_INSTALL_DIR}/lib/libavdevice.a
    ${FFMPEG_INSTALL_DIR}/lib/libavfilter.a
    ${FFMPEG_INSTALL_DIR}/lib/libavformat.a
    ${FFMPEG_INSTALL_DIR}/lib/libavcodec.a
    ${FFMPEG_INSTALL_DIR}/lib/libswscale.a
    ${FFMPEG_INSTALL_DIR}/lib/libswresample.a
    ${FFMPEG_INSTALL_DIR}/lib/libavutil.a
)

add_library(ffmpeg INTERFACE)
target_include_directories(ffmpeg INTERFACE ${FFMPEG_INCLUDE_DIRS})
target_link_libraries(ffmpeg INTERFACE ${FFMPEG_LIBRARIES})

# Кроссплатформенная линковка для ffmpeg INTERFACE
if(APPLE)
    target_link_libraries(ffmpeg INTERFACE
        "-framework Metal"
        "-framework VideoToolbox"
        "-framework CoreImage"
        "-framework AppKit"
        "-framework CoreFoundation"
        "-framework CoreMedia"
        "-framework CoreVideo"
        "-framework CoreServices"
        "-framework Security"
        "-framework AudioToolbox"
        z
        iconv
    )
elseif(UNIX)
    target_link_libraries(ffmpeg INTERFACE
        z
        bz2
        lzma
        zstd
        m
        pthread
        iconv
    )
endif()

add_dependencies(ffmpeg ffmpeg_ext)