add_subdirectory(img)

add_library(VTL_media_container OBJECT
    audio/VTL_audio_data.c
    audio/infra/VTL_audio_read.c
    audio/infra/VTL_audio_write.c
    video/VTL_video_data.c
    sub/VTL_sub_data.c
    sub/VTL_sub_convert.c
    sub/VTL_sub_parse.c
    sub/VTL_sub_style.c
    sub/infra/VTL_sub_read.c
    sub/infra/VTL_sub_write.c
)

# VTL_sub_burn.c использует POSIX realpath() — под Windows её нет.
# Подключать только под UNIX, пока исходник не сделают кросс-платформенным.
if(NOT WIN32)
    target_sources(VTL_media_container PRIVATE sub/VTL_sub_burn.c)
endif()

target_include_directories(VTL_media_container PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(VTL_media_container
    PUBLIC
        VTL_utils
        VTL_img
        ffmpeg
)

set_target_properties(VTL_media_container PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

if(MSVC)
    target_compile_options(VTL_media_container PRIVATE /W4 /utf-8)
else()
    target_compile_options(VTL_media_container PRIVATE
        -Wall -Wextra
        -Wno-error=implicit-function-declaration
        -Wno-error=incompatible-pointer-types
        -Wno-error=int-conversion
    )
endif()
