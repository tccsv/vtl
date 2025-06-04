#ifndef _VTL_AUDIO_GEN_H
#define _VTL_AUDIO_GEN_H

#ifdef __cplusplus
extern "C"
{
#endif



#include <VTL/media_container/audio/VTL_audio.h>
#include <VTL/utils/VTL_file.h>
#include <VTL/VTL_app_result.h>
#include <stdlib.h>
#include <string.h>


VTL_AppResult VTL_audio_Gen(const VTL_Filename file_name, const VTL_audio_Configs* p_configs);

#ifdef __cplusplus
}
#endif

#endif



