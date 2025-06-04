#ifndef _VTL_AUDIO_CONFIGS_FOR_GEN_INIT_H
#define _VTL_AUDIO_CONFIGS_FOR_GEN_INIT_H

#ifdef __cplusplus
extern "C"
{
#endif


#include <VTL/content_platform/tg/media_container_params/VTL_content_platform_tg_params_audio.h>
#include <VTL/media_container/audio/VTL_audio.h>
#include <VTL/VTL_content_platform_flags.h>
#include <VTL/utils/VTL_file.h>
#include <VTL/VTL_app_result.h>

#include <stdlib.h>
#include <stddef.h>

typedef struct _VTL_audio_configs_platforms_Indices
{
    VTL_ContentPlatform* data;
    size_t length;
} VTL_audio_configs_platforms_Indices;

VTL_AppResult VTL_audio_configs_for_gen_Init(VTL_audio_Configs** pp_new_configs, 
                                    VTL_audio_configs_platforms_Indices** pp_indices, 
                                    const VTL_Filename src_file_name, const VTL_content_platform_flags flags);


#ifdef __cplusplus
}
#endif

#endif