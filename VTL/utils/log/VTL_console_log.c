#include <VTL/utils/log/VTL_console_log.h>

#define VTL_console_output(x) printf_s("%s", x) 

#define VTL_CONSOLE_OUT_INPUT_FILE_IS_MISSING_ERR_TEXT "Input file is missing."
#define VTL_CONSOLE_OUT_INPUT_FILE_IS_BUSY_ERR_TEXT "Input file is busy."
// TO DO Добавить разных ошибок

void VTL_console_out_PotencialErr(VTL_AppResult app_result)
{
    if(app_result == VTL_res_kOk)
    {
        return;
    }
    else if(app_result == VTL_res_video_fs_r_kMissingFileErr)
    {
        VTL_console_output(VTL_CONSOLE_OUT_INPUT_FILE_IS_MISSING_ERR_TEXT);
    }
    else if(app_result == VTL_res_video_fs_r_kFileIsBusyErr)
    {
        VTL_console_output(VTL_CONSOLE_OUT_INPUT_FILE_IS_BUSY_ERR_TEXT);
    }
}