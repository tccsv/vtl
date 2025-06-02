#ifndef _VTL_APP_RESULT_H
#define _VTL_APP_RESULT_H

#ifdef __cplusplus
extern "C"
{
#endif



typedef enum _VTL_AppResult 
{
    VTL_res_kOk = 0,
    
    
    VTL_res_video_fs_r_kMissingFileErr = 1,
    VTL_res_video_fs_r_kFileIsBusyErr,

    VTL_res_video_fs_w_kFileIsBusyErr = 10,

    VTL_res_ffmpeg_kFormatError,
    VTL_res_ffmpeg_kStreamError,
    VTL_res_ffmpeg_kCodecError,
    VTL_res_ffmpeg_kIOError,
    VTL_res_ffmpeg_kInitError,
    VTL_res_ffmpeg_kMemoryError,
    VTL_res_ffmpeg_kConversionError,
    VTL_res_kEncoderNeedsMoreFrames,
    VTL_res_kEncoderFlushComplete,
    
} VTL_AppResult;




#ifdef __cplusplus
}
#endif


#endif