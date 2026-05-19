#ifndef _VTL_APP_RESULT_H
#define _VTL_APP_RESULT_H

#ifdef __cplusplus
extern "C"
{
#endif



typedef enum _VTL_AppResult
{
    VTL_res_kOk = 0,
    VTL_res_kErr = -1,
    VTL_res_kInvalidParamErr = -2,
    VTL_res_kMemAllocErr = -3,
    VTL_res_kFileOpenErr = -4,
    VTL_res_kFileReadErr = -5,
    VTL_res_kFileWriteErr = -6,

    VTL_res_video_fs_r_kMissingFileErr = 1,
    VTL_res_video_fs_r_kFileIsBusyErr,

    VTL_res_video_fs_w_kFileIsBusyErr = 10,

    VTL_res_text_io_r_kInvalidArgument = 20,
    VTL_res_text_ms_w_kOutOfMemory = 30,

    /* добавлено из feature/impl-video-edit (видео-редактор Дмитрия) */
    VTL_res_ffmpeg_kFormatError,
    VTL_res_ffmpeg_kStreamError,
    VTL_res_ffmpeg_kCodecError,
    VTL_res_ffmpeg_kIOError,
    VTL_res_ffmpeg_kInitError,
    VTL_res_ffmpeg_kMemoryError,
    VTL_res_ffmpeg_kConversionError,
    VTL_res_kEncoderNeedsMoreFrames,
    VTL_res_kEncoderFlushComplete,

    /* добавлено из fedorov-subtitles-macOS (субтитры Федорова) */
    VTL_res_kUnsupportedFormat = 50,
    VTL_res_kParseError,
    VTL_res_kMemoryError,
    VTL_res_kArgumentError,
    VTL_res_kIOError,
    VTL_res_kUnknownError,

    VTL_res_kNullArgument = 100,
    VTL_res_kAllocError,
    VTL_res_kEndOfFile,

    VTL_res_kSubtitleTimeParseError = 110,
    VTL_res_kSubtitleFormatError,
    VTL_res_kSubtitleTextOverflow,
    VTL_res_kSubInit,

    VTL_res_burn_kOpenInputFileError = 200,
    VTL_res_burn_kCreateOutputContextError,
    VTL_res_burn_kCreateOutputStreamError,
    VTL_res_burn_kInitFiltersError,
    VTL_res_burn_kSetupVideoEncoderError,
    VTL_res_burn_kSetupAudioEncoderError,
    VTL_res_burn_kOpenOutputFileError,
    VTL_res_burn_kWriteHeaderError,

    VTL_res_convert_kOpenInputFileError = 220,
    VTL_res_convert_kReadMetaError,
    VTL_res_convert_kOpenOutputFileError,
    VTL_res_convert_kWritePartError,
    VTL_res_convert_kAllocError,

    VTL_res_style_kJsonParseError = 240,

    VTL_res_vimeo_kMissingToken = 260,
    VTL_res_vimeo_kHttpPostError,
    VTL_res_vimeo_kHttpPatchError,
    VTL_res_vimeo_kTusUploadError,
    VTL_res_vimeo_kJsonParseError,
    VTL_res_vimeo_kMissingUploadLink

} VTL_AppResult;




#ifdef __cplusplus
}
#endif


#endif