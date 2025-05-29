#include <stdio.h>
#include <VTL/media_container/audio/infra/VTL_audio_read.h>
#include <VTL/VTL_app_result.h>

VTL_AppResult VTL_audio_read_OpenSource(VTL_audio_File** pp_audio_file, const VTL_Filename srt_file_name)
{
    if (!pp_audio_file || !srt_file_name) return VTL_res_video_fs_r_kMissingFileErr;
    FILE* f = fopen(srt_file_name, "rb");
    if (!f) return VTL_res_video_fs_r_kMissingFileErr;
    *pp_audio_file = f;
    return VTL_res_kOk;
}

VTL_AppResult VTL_audio_read_CloseSource(VTL_audio_File* p_audio_file)
{
    if (!p_audio_file) return VTL_res_video_fs_r_kMissingFileErr;
    fclose(p_audio_file);
    return VTL_res_kOk;
}

VTL_AppResult VTL_audio_ReadMetaData(VTL_audio_MetaData* p_audio_meta_data, VTL_audio_File* p_audio_file,
                                            VTL_audio_file_op_Result* p_file_op_result)
{
    if (!p_audio_meta_data || !p_audio_file) return VTL_res_video_fs_r_kMissingFileErr;
    size_t n = fread(p_audio_meta_data, sizeof(VTL_audio_MetaData), 1, p_audio_file);
    if (p_file_op_result) *p_file_op_result = (n == 1) ? VTL_AUDIO_FILE_OP_OK : VTL_AUDIO_FILE_EOF;
    return (n == 1) ? VTL_res_kOk : VTL_res_video_fs_r_kMissingFileErr;
}

VTL_AppResult VTL_audio_ReadPart(VTL_audio_Data** pp_audio_part, const VTL_audio_MetaData* p_audio_meta_data, 
                                    VTL_audio_File* p_audio_file, VTL_audio_file_op_Result* p_file_op_result)
{
    if (!pp_audio_part || !p_audio_meta_data || !p_audio_file) return VTL_res_video_fs_r_kMissingFileErr;
    *pp_audio_part = (VTL_audio_Data*)malloc(sizeof(VTL_audio_Data));
    if (!*pp_audio_part) return VTL_res_video_fs_r_kMissingFileErr;
    (*pp_audio_part)->data_size = p_audio_meta_data->data_size;
    (*pp_audio_part)->data = (char*)malloc((*pp_audio_part)->data_size);
    if (!(*pp_audio_part)->data) { free(*pp_audio_part); return VTL_res_video_fs_r_kMissingFileErr; }
    size_t n = fread((*pp_audio_part)->data, 1, (*pp_audio_part)->data_size, p_audio_file);
    if (p_file_op_result) *p_file_op_result = (n == (*pp_audio_part)->data_size) ? VTL_AUDIO_FILE_OP_OK : VTL_AUDIO_FILE_EOF;
    return (n == (*pp_audio_part)->data_size) ? VTL_res_kOk : VTL_res_video_fs_r_kMissingFileErr;
}

VTL_AppResult VTL_audio_meta_data_InitFromSource(VTL_audio_MetaData* p_audio_meta_data, 
                                                    const VTL_Filename src_file_name)
{
    if (!p_audio_meta_data || !src_file_name) return VTL_res_video_fs_r_kMissingFileErr;
    VTL_audio_File* p_audio_r_file = NULL;
    VTL_audio_file_op_Result audio_r_op_res;
    if (VTL_audio_read_OpenSource(&p_audio_r_file, src_file_name) != VTL_res_kOk) return VTL_res_video_fs_r_kMissingFileErr;
    VTL_AppResult res = VTL_audio_ReadMetaData(p_audio_meta_data, p_audio_r_file, &audio_r_op_res);
    VTL_audio_read_CloseSource(p_audio_r_file);
    return res;
}
