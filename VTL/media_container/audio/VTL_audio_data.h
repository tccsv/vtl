#ifndef _VTL_AUDIO_DATA_H
#define _VTL_AUDIO_DATA_H

#ifdef __cplusplus
extern "C"
{
#endif


#include <VTL/utils/VTL_buffer_data.h>
#include <VTL/VTL_app_result.h>
#include <stdint.h> 


typedef uint_least32_t VTL_audio_Bitrate;
typedef uint_least32_t VTL_audio_SampleRate;
typedef uint_least8_t VTL_audio_NumChannels;
typedef uint_least8_t VTL_audio_Volume;
// // typedef uint32_t VTL_audio_Frequency;


// //TO DO поизучать, чем заменить заглушку для фазы
// typedef int VTL_audio_Phase;

// //TO DO поизучать, чем заменить заглушку для эквализации
// typedef int VTL_audio_EQ;



typedef enum _VTL_audio_Codec
{
    VTL_audio_codec_kDefault = 0,
    VTL_audio_codec_kAAC,
    VTL_audio_codec_kVorbis,
    VTL_audio_codec_kOpus,
    VTL_audio_codec_kFLAC,

    //TODO Добавить кодеков
} VTL_audio_Codec;

#define VTL_AUDIO_BITRATE_MIN
#define VTL_AUDIO_BITRATE_MAX
#define VTL_AUDIO_SAMPLE_RATE_MIN
#define VTL_AUDIO_SAMPLE_RATE_MAX
#define VTL_AUDIO_VOLUME_MIN
#define VTL_AUDIO_VOLUME_MAX


typedef struct _VTL_audio_Params
{
    VTL_audio_Bitrate bitrate;
    VTL_audio_Codec codec;
    VTL_audio_SampleRate sample_rate;
    VTL_audio_NumChannels num_channels;
    VTL_audio_Volume volume;
} VTL_audio_Params;

// typedef struct _VTL_audio_Echo
// {
//     //TODO Продумать
//     int x;
// } VTL_audio_Echo;

// typedef struct _VTL_audio_Reverb
// {
//     //TODO Продумать
//     int x;
// } VTL_audio_Reverb;

// typedef struct _VTL_audio_Compression
// {
//     //TODO Продумать
//     int x;
// } VTL_audio_Compression;

// typedef struct _VTL_audio_FrequencyFilter
// {
//     //TODO Продумать
//     int x;
// } VTL_audio_FrequencyFilter;

// typedef struct _VTL_audio_NoiseEffects
// {
//     //TODO Продумать
//     int x;
// } VTL_audio_NoiseEffects;

// typedef struct _VTL_audio_StereoEffects
// {
//     //TODO Продумать
//     int x;
// } VTL_audio_StereoEffects;

// typedef struct _VTL_audio_BassBoost
// {
//     //TODO Продумать
//     int x;
// } VTL_audio_BassBoost;


typedef struct _VTL_audio_MetaData
{
    size_t data_size;
    VTL_audio_Params params;
} VTL_audio_MetaData;

typedef VTL_BufferData VTL_audio_Data;


typedef struct _VTL_Audio
{
    VTL_audio_MetaData meta_data;
    VTL_audio_Data* p_current_part;
} VTL_Audio;


// VTL_AppResult VTL_audio_Data_Decode(VTL_audio_Data **pp_audio_part, const VTL_audio_Params* p_params);
VTL_AppResult VTL_audio_Data_Encode(VTL_audio_Data **pp_audio_part, const VTL_audio_Params* p_old_params, 
                                        const VTL_audio_Params* p_new_params);

VTL_AppResult VTL_audio_params_BitrateSet(VTL_audio_Params* p_params, const VTL_audio_Bitrate bitrate);
VTL_AppResult VTL_audio_params_CodecSet(VTL_audio_Params* p_params, const VTL_audio_Codec codec);
VTL_AppResult VTL_audio_params_SampleRateSet(VTL_audio_Params* p_params, const VTL_audio_SampleRate sample_rate);
VTL_AppResult VTL_audio_params_NumChannelsSet(VTL_audio_Params* p_params, const VTL_audio_NumChannels numChannels);
VTL_AppResult VTL_audio_params_VolumeSet(VTL_audio_Params* p_params, const VTL_audio_Volume volume);





#ifdef __cplusplus
}
#endif


#endif