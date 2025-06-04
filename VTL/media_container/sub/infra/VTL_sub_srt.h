#ifndef _VTL_SUB_SRT_H
#define _VTL_SUB_SRT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/media_container/sub/VTL_sub_data.h>
#include <VTL/VTL_app_result.h>
#include <stddef.h>

VTL_AppResult VTL_sub_srt_Conversion(VTL_Sub** pp_sub, const char* p_raw_srt_data, size_t data_size);

#ifdef __cplusplus
}
#endif


#endif