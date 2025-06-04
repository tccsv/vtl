#ifndef _VTL_AUDIO_WRITE_H
#define _VTL_AUDIO_WRITE_H

#ifdef __cplusplus
extern "C"
{
#endif


#include <VTL/media_container/audio/infra/VTL_audio_config.h>
#include <VTL/media_container/audio/infra/VTL_audio_file.h>
#include <VTL/media_container/audio/VTL_audio_data.h>
#include <VTL/utils/VTL_file.h>
#include <VTL/VTL_app_result.h>


VTL_AppResult VTL_audio_OpenOutput(VTL_audio_File** pp_outputs, const VTL_Filename output_name);
VTL_AppResult VTL_audio_WritePart(const VTL_audio_Data* p_audio_part, VTL_audio_File* p_output);
VTL_AppResult VTL_audio_CloseOutput(VTL_audio_File* p_outputs);


#ifdef __cplusplus
}
#endif

#endif
