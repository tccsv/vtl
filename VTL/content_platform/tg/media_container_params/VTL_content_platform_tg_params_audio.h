#ifndef _VTL_CONTENT_PLATFORM_TG_PARAMS_AUDIO_H
#define _VTL_CONTENT_PLATFORM_TG_PARAMS_AUDIO_H

#ifdef __cplusplus
extern "C"
{
#endif


#include <VTL/media_container/audio/VTL_audio_data.h>
#include <string.h>

void VTL_audio_tg_SetParams(VTL_audio_Params *p_new_params, const VTL_audio_Params *p_old_params);

#ifdef __cplusplus
}
#endif

#endif



