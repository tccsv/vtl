#ifndef _VTL_CONTENT_PLATFORM_FLAGS_H
#define _VTL_CONTENT_PLATFORM_FLAGS_H


#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

typedef enum _VTL_ContentPlatform
{
        VTL_content_platform_kW = 0,
        VTL_content_platform_kTG,
        VTL_content_platform_kYT,
        VTL_content_platform_kBiliBili,
        VTL_content_platform_kR,
        VTL_content_platform_kVK,
        VTL_content_platform_kTiktok,

        //VTL_content_platform_kX необходимо оставлять всегда последним
        VTL_content_platform_kX

} VTL_ContentPlatform;

#define VTL_CONTENT_PLATFORM_DEFAULT !UINT_LEAST32_MAX 
#define VTL_CONTENT_PLATFORM_W ( 1 << VTL_content_platform_kW)
#define VTL_CONTENT_PLATFORM_R ( 1 << VTL_content_platform_kR)
#define VTL_CONTENT_PLATFORM_TG (1 << VTL_content_platform_kTG)
#define VTL_CONTENT_PLATFORM_YT (1 << VTL_content_platform_kYT)
#define VTL_CONTENT_PLATFORM_BILIBILI (1 << VTL_content_platform_kBiliBili)
#define VTL_CONTENT_PLATFORM_X (1 << VTL_content_platform_kX)
#define VTL_CONTENT_PLATFORM_TIKTOK (1 << VTL_content_platform_kTiktok)

#define VTL_CONTENT_PLATFORM_VK (1 << VTL_content_platform_kX)
#define VTL_CONTENT_PLATFORM_MAX_NUM (VTL_content_platform_kX + 1)

#define VTL_CONTENT_PLATFORM_ALL UINT_LEAST32_MAX 
 
#define VTL_CONTENT_PLATFORM_VIDEO_W_TEXT (VTL_CONTENT_PLATFORM_W & VTL_CONTENT_PLATFORM_TG & VTL_CONTENT_PLATFORM_YT \
        & VTL_CONTENT_PLATFORM_BILIBILI & VTL_CONTENT_PLATFORM_X & VTL_CONTENT_PLATFORM_VK)

#define VTL_CONTENT_PLATFORM_VIDEO_TEXT (VTL_CONTENT_PLATFORM_W & VTL_CONTENT_PLATFORM_TG & VTL_CONTENT_PLATFORM_YT) 

typedef int VTL_content_platform_flags;


bool VTL_content_platform_flags_CheckTg(const VTL_content_platform_flags flags);
bool VTL_content_platform_flags_CheckW(const VTL_content_platform_flags flags);



#ifdef __cplusplus
}
#endif


#endif