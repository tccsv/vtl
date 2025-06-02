#ifndef VTL_VIDEO_EDITOR_H
#define VTL_VIDEO_EDITOR_H

#include "VTL/VTL_app_result.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

typedef struct {
    enum AVCodecID codec_id;    // AV_CODEC_ID_AAC, AV_CODEC_ID_MP3 и др.
    int bit_rate;               // Битрейт (128000, 192000 и т.д.)
    int sample_rate;            // Частота дискретизации (44100, 48000)
    int channels;               // Количество каналов (1 - моно, 2 - стерео)
    uint64_t channel_layout;    // Расположение каналов (AV_CH_LAYOUT_MONO и др.)
    float volume;               // Уровень громкости (1.0 - оригинал)
    int profile;                // Профиль кодека (для AAC)
    enum AVSampleFormat sample_fmt; // Формат сэмплов
} VTL_AudioParams;

extern const VTL_AudioParams VTL_AUDIO_PRESET_AAC_DEFAULT;
extern const VTL_AudioParams VTL_AUDIO_PRESET_MP3_HQ;

VTL_AppResult VTL_process_video(
    const char* input_path,
    const char* output_path,
    const VTL_AudioParams* audio_params
);

#endif // VTL_VIDEO_EDITOR_H