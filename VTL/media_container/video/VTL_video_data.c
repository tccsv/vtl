#include <VTL/media_container/video/VTL_video_data.h>


// VTL_AppResult VTL_video_data_Decode(VTL_video_Data **pp_video_part, const VTL_video_Params* p_params)
// {
//     return VTL_res_kOk;
// }

VTL_AppResult VTL_video_part_Encode(VTL_video_Data **pp_video_part, const VTL_video_Params* p_old_params, 
                                        const VTL_video_Params* p_new_params)
{
    return VTL_res_kOk;
}

VTL_AppResult VTL_video_params_BitrateSet(VTL_video_Params* p_params, const VTL_video_Bitrate bitrate)
{
    return VTL_res_kOk;
}

VTL_AppResult VTL_video_params_FpsSet(VTL_video_Params* p_params, const VTL_video_FPS bitrate)
{
    return VTL_res_kOk;
}

VTL_AppResult VTL_video_params_CodecSet(VTL_video_Params* p_params, const VTL_video_Codec codec)
{
    return VTL_res_kOk;
}

VTL_AppResult VTL_video_params_ResolutionSet(VTL_video_Params* p_params, const VTL_video_Resolution resolution)
{
    return VTL_res_kOk;
}

VTL_AppResult VTL_video_params_ContainerTypeSet(VTL_video_Params* p_params, const VTL_video_ContainerType container_type)
{
    return VTL_res_kOk;
}

