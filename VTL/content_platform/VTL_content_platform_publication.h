#ifndef _VTL_CONTENT_PLATFORM_PUBLICATION_H
#define _VTL_CONTENT_PLATFORM_PUBLICATION_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/content_platform/infra/VTL_content_platform_infra.h>
#include <VTL/content_platform/tg/VTL_content_platform_tg_publication.h>
#include <VTL/VTL_content_platform_flags.h>
#include <VTL/VTL_publication_markup_text_flags.h>

#include <VTL/VTL_app_result.h>



VTL_AppResult VTL_content_platform_PublicateMarkedTexts(const VTL_publication_marked_text_Configs* p_configs,
    const VTL_content_platform_flags flags);

VTL_AppResult VTL_content_platform_PublicateAudioWithMarkedText(VTL_audio_Configs* p_audio_configs,
    VTL_audio_configs_platforms_Indices *p_indices, VTL_publication_marked_text_Configs* p_text_configs,
    const VTL_content_platform_flags flags);
#ifdef __cplusplus
}
#endif


#endif