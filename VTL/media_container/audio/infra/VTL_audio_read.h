#ifndef _VTL_AUDIO_READ_H
#define _VTL_AUDIO_READ_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/media_container/audio/infra/VTL_audio_file.h>
#include <VTL/media_container/audio/VTL_audio_data.h>
#include <VTL/utils/VTL_file.h>
#include <VTL/VTL_app_result.h>


VTL_AppResult VTL_audio_read_OpenSource(VTL_audio_File** pp_audio_file, const VTL_Filename srt_file_name);
VTL_AppResult VTL_audio_read_CloseSource(VTL_audio_File* p_audio_file);
VTL_AppResult VTL_audio_ReadMetaData(VTL_audio_MetaData* p_audio_meta_data, VTL_audio_File* p_audio_file,
                                            VTL_audio_file_op_Result* p_file_op_result);
VTL_AppResult VTL_audio_ReadPart(VTL_audio_Data** pp_audio_part, const VTL_audio_MetaData* p_audio_meta_data, 
                                    VTL_audio_File* p_audio_file, VTL_audio_file_op_Result* p_file_op_result);
VTL_AppResult VTL_audio_meta_data_InitFromSource(VTL_audio_MetaData* p_audio_meta_data, 
                                                    const VTL_Filename src_file_name);

#ifdef __cplusplus
}
#endif

#endif
