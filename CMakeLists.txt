cmake_minimum_required(VERSION 3.25.0)

project(VTL C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Add compiler flags
add_compile_options(-Wno-implicit-function-declaration)

# Add system include paths
include_directories(SYSTEM /usr/include)

include(cmake/FFmpeg.cmake)

set(AppName "VTL")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/app)

set(VTL_CMAKE_UTILS_SRC_FILES 
    VTL/utils/VTL_time.c
    VTL/utils/VTL_file.c
    VTL/utils/VTL_encryptor.c
    VTL/utils/log/VTL_console_log.c
    VTL/utils/db/VTL_db_credentals.c
)

set(VTL_CMAKE_USER_SRC_FILES
    VTL/user/VTL_user_data.c
    VTL/user/history/VTL_user_history_data.c
    VTL/user/history/db/VTL_user_history_save.c    
)

set(VTL_CMAKE_PUBLICATION_TEXT_SRC_FILES 
    VTL/publication/text/VTL_publication_text_data.c
    VTL/publication/text/VTL_publication_text_op.c
    VTL/publication/text/VTL_publication_markup_text_flags_check.c
    VTL/publication/text/infra/VTL_publication_text_read.c
    VTL/publication/text/infra/VTL_publication_text_write.c
)

set(VTL_CMAKE_PUBLICATION_SRC_FILES 
    ${VTL_CMAKE_PUBLICATION_TEXT_SRC_FILES}
    VTL/content_platform/tg/VTL_content_platform_tg_net.c
    VTL/content_platform/VTL_content_platform_publication.c
    VTL/publication/VTL_publication.c    
)

set(VTL_CMAKE_MEDIA_CONTAINER_AUDIO_SRC_FILES 
    VTL/media_container/audio/VTL_audio_data.c
    VTL/media_container/audio/infra/VTL_audio_read.c
    VTL/media_container/audio/infra/VTL_audio_write.c
)

set(VTL_CMAKE_MEDIA_CONTAINER_VIDEO_SRC_FILES 
    VTL/media_container/video/VTL_video_data.c
)

set(VTL_CMAKE_MEDIA_CONTAINER_IMG_SRC_FILES 
    VTL/media_container/img/VTL_img_data.c
    VTL/media_container/img/infra/VTL_img_read.c
    VTL/media_container/img/infra/VTL_img_write.c
    VTL/media_container/img/infra/VTL_img_file.c
    VTL/media_container/img/infra/VTL_img_config.c
)

set(VTL_CMAKE_MEDIA_CONTAINER_SRC_FILES
    ${VTL_CMAKE_MEDIA_CONTAINER_AUDIO_SRC_FILES}
    ${VTL_CMAKE_MEDIA_CONTAINER_VIDEO_SRC_FILES}
    ${VTL_CMAKE_MEDIA_CONTAINER_IMG_SRC_FILES}
)

set(VTL_CMAKE_CONTENT_PLATFORM_TG_SRC_FILES
    VTL/content_platform/tg/media_container_params/VTL_content_platform_tg_params_audio.c
    VTL/content_platform/tg/media_container_params/VTL_content_platform_tg_params_sub.c
    VTL/content_platform/tg/media_container_params/VTL_content_platform_tg_params_video.c
    VTL/content_platform/infra/text/VTL_text_configs_for_gen_init.c
)

set(VTL_CMAKE_CONTENT_PLATFORM_SRC_FILES
    ${VTL_CMAKE_CONTENT_PLATFORM_TG_SRC_FILES}
    VTL/content_platform/VTL_content_platform_flags_check.c
    VTL/content_platform/infra/audio/VTL_audio_gen.c
    VTL/content_platform/infra/audio/VTL_audio_configs_for_gen_init.c
    VTL/content_platform/infra/text/VTL_publication_text_gen.c
    VTL/content_platform/VTL_content_platform_publication.c
)

add_executable(${AppName}
    ${VTL_CMAKE_UTILS_SRC_FILES}
    ${VTL_CMAKE_USER_SRC_FILES}
    ${VTL_CMAKE_PUBLICATION_SRC_FILES}
    ${VTL_CMAKE_MEDIA_CONTAINER_SRC_FILES}
    ${VTL_CMAKE_CONTENT_PLATFORM_SRC_FILES}
    main.c
)

# Link FFmpeg
target_link_libraries(${AppName} PRIVATE 
    ffmpeg_libs
    m           # математическая библиотека
    va          # VAAPI
    va-drm      # VAAPI DRM
    va-x11      # VAAPI X11
    drm         # Direct Rendering Manager
    X11         # X11
    vdpau       # VDPAU
    z           # zlib
    bz2         # bzip2
    lzma        # xz/lzma
    lz4         # lz4
    zstd        # zstd
    x264        # H.264
    x265        # H.265
    vpx         # VP8/VP9
    aom         # AV1
    opus        # Opus audio
    vorbis      # Vorbis audio
    ogg         # Ogg container
    swresample  # FFmpeg resampling
    swscale     # FFmpeg scaling
)

target_include_directories(${AppName} 
    PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/VTL
    ${CMAKE_SOURCE_DIR}/VTL/user
    ${CMAKE_SOURCE_DIR}/VTL/user/history
    SYSTEM
    /usr/include
)

enable_testing()

add_subdirectory(vtl_tests)
