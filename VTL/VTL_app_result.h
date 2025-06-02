#ifndef _VTL_APP_RESULT_H
#define _VTL_APP_RESULT_H

#ifdef __cplusplus
extern "C"
{
#endif



typedef enum _VTL_AppResult 
{
    VTL_res_kOk = 0,
    
    // Общие ошибки
    VTL_res_kInvalidParamErr = -1,
    VTL_res_kMemAllocErr = -2,
    VTL_res_kFileOpenErr = -3,
    VTL_res_kFileReadErr = -4,
    VTL_res_kFileWriteErr = -5,
    
    // Ошибки файловой системы
    VTL_res_video_fs_r_kMissingFileErr = 1,
    VTL_res_video_fs_r_kFileIsBusyErr,

    VTL_res_video_fs_w_kFileIsBusyErr = 10,


    
} VTL_AppResult;




#ifdef __cplusplus
}
#endif


#endif