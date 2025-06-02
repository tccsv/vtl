#ifndef VTL_FRAME_INTERPOLATION_H
#define VTL_FRAME_INTERPOLATION_H

#include "VTL/VTL_app_result.h"

#include <stdint.h>

// Добавляем объявление типа результата
// typedef enum {
//     VTL_res_kOk = 0,
//     VTL_res_video_fs_r_kMissingFileErr,
//     VTL_res_video_fs_w_kFileIsBusyErr,
//     VTL_res_ffmpeg_kFormatError,
//     VTL_res_ffmpeg_kStreamError,
//     VTL_res_ffmpeg_kCodecError,
//     VTL_res_ffmpeg_kIOError,
//     VTL_res_ffmpeg_kInitError
// } VTL_AppResult;

typedef struct {
    int width;
    int height;
    int frame_rate;
    int target_frame_rate;
    float optical_flow_scale;
    int gpu_device_id;
} VTL_InterpolationParams;

VTL_AppResult VTL_interpolate_video_frames(
    const char* input_path,
    const char* output_path,
    const VTL_InterpolationParams* interpol_params
);

//VTL_Interpolator* VTL_init_interpolator(const VTL_InterpolationParams* params);
// int VTL_interpolate_frames(void* interpolator, const uint8_t* prev_frame, const uint8_t* next_frame, uint8_t* output_frames, int num_frames_to_generate);
void VTL_free_interpolator(void* interpolator);
VTL_AppResult VTL_transcode_only(const char* input_path, const char* output_path);

#endif // VTL_FRAME_INTERPOLATION_H