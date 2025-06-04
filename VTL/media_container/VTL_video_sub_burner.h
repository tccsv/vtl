#ifndef _VTL_VIDEO_SUB_BURNER_H
#define _VTL_VIDEO_SUB_BURNER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/media_container/sub/VTL_sub_data.h>
#include <VTL/media_container/video/VTL_video_data.h>
#include <VTL/utils/VTL_file.h>
#include <VTL/VTL_app_result.h>


VTL_AppResult VTL_video_sub_BurnIn(VTL_Filename video_file_name, VTL_Sub sub);



#ifdef __cplusplus
}
#endif


#endif