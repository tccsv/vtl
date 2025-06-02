#include <stdio.h>
#include <stdlib.h>
#include <VTL/media_container/audio/infra/VTL_audio_read.h>
#include <VTL/VTL_app_result.h>

VTL_AppResult VTL_audio_OpenOutput(VTL_audio_File** pp_outputs, const VTL_Filename output_name)
{
    if (!pp_outputs || !output_name) return VTL_res_video_fs_r_kMissingFileErr;
    FILE* f = fopen(output_name, "wb");
    if (!f) return VTL_res_video_fs_r_kMissingFileErr;
    *pp_outputs = f;
    return VTL_res_kOk;
}

VTL_AppResult VTL_audio_WritePart(const VTL_audio_Data* p_audio_part, VTL_audio_File* p_outputs)
{
    if (!p_audio_part || !p_outputs) return VTL_res_video_fs_r_kMissingFileErr;
    size_t n = fwrite(p_audio_part->data, 1, p_audio_part->data_size, p_outputs);
    return (n == p_audio_part->data_size) ? VTL_res_kOk : VTL_res_video_fs_r_kMissingFileErr;
}

VTL_AppResult VTL_audio_CloseOutput(VTL_audio_File* p_outputs)
{
    if (!p_outputs) return VTL_res_video_fs_r_kMissingFileErr;
    fclose(p_outputs);
    return VTL_res_kOk;
}