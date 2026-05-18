#include <VTL/media_container/audio/VTL_audio_data.h>


VTL_AppResult VTL_audio_Data_Encode(VTL_audio_Data **pp_audio_part, const VTL_audio_Params* p_old_params, 
                                        const VTL_audio_Params* p_new_params)
{
    return VTL_res_kOk;
}

VTL_AppResult VTL_audio_params_BitrateSet(VTL_audio_Params* p_params, const VTL_audio_Bitrate bitrate)
{
    return VTL_res_kOk;
}

VTL_AppResult VTL_audio_params_CodecSet(VTL_audio_Params* p_params, const VTL_audio_Codec codec)
{
    return VTL_res_kOk;
}

VTL_AppResult VTL_audio_params_SampleRateSet(VTL_audio_Params* p_params, const VTL_audio_SampleRate sample_rate)
{
    return VTL_res_kOk;
}

VTL_AppResult VTL_audio_params_NumChannelsSet(VTL_audio_Params* p_params, const VTL_audio_NumChannels numChannels)
{
    return VTL_res_kOk;
}

VTL_AppResult VTL_audio_params_VolumeSet(VTL_audio_Params* p_params, const VTL_audio_Volume volume)
{
    return VTL_res_kOk;
}