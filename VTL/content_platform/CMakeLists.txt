add_library(VTL_content_platform OBJECT
    VTL_content_platform_flags_check.c
    VTL_content_platform_publication.c
    tg/VTL_content_platform_tg_net.c
    tg/media_container_params/VTL_content_platform_tg_params_audio.c
    tg/media_container_params/VTL_content_platform_tg_params_sub.c
    tg/media_container_params/VTL_content_platform_tg_params_video.c
    vimeo/VTL_content_platform_vimeo_net.c
    vimeo/media_container_params/VTL_content_platform_vimeo_params_video.c
    reddit/VTL_content_platform_reddit_net.c
    infra/audio/VTL_audio_gen.c
    infra/audio/VTL_audio_configs_for_gen_init.c
    infra/text/VTL_publication_text_gen.c
    infra/text/VTL_text_configs_for_gen_init.c
)

target_include_directories(VTL_content_platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(VTL_content_platform
    PUBLIC
        VTL_publication
        VTL_media_container
        VTL_user
        VTL_utils
)

set_target_properties(VTL_content_platform PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

if(MSVC)
    target_compile_options(VTL_content_platform PRIVATE /W4 /utf-8)
else()
    target_compile_options(VTL_content_platform PRIVATE
        -Wall -Wextra
        -Wno-error=implicit-function-declaration
        -Wno-error=incompatible-pointer-types
        -Wno-error=int-conversion
    )
endif()
