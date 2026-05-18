#ifndef _VTL_SUB_READ_H
#define _VTL_SUB_READ_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/media_container/sub/infra/VTL_sub_srt.h>
#include <VTL/media_container/sub/VTL_sub_data.h>
#include <VTL/utils/VTL_file.h>



VTL_AppResult VTL_sub_InitFromASSFile(VTL_Sub** pp_sub, const VTL_Filename ass_file_name);
VTL_AppResult VTL_sub_InitFromSBVFile(VTL_Sub** pp_sub, const VTL_Filename sbv_file_name);
VTL_AppResult VTL_sub_InitFromSRTFile(VTL_Sub** pp_sub, const VTL_Filename srt_file_name);
VTL_AppResult VTL_sub_InitFromTTMLFile(VTL_Sub** pp_sub, const VTL_Filename ttml_file_name);
VTL_AppResult VTL_sub_InitFromVTTFile(VTL_Sub** pp_sub, const VTL_Filename vtt_file_name);



#ifdef __cplusplus
}
#endif


#endif