#ifndef _VTL_CONTENT_PLATFORM_FLAGS_CHECK_H
#define _VTL_CONTENT_PLATFORM_FLAGS_CHECK_H

#ifdef __cplusplus
extern "C"
{
#endif


#include <VTL/VTL_content_platform_flags.h>
#include <stdbool.h>

bool VTL_content_platform_flags_CheckTg(const VTL_content_platform_flags flags);
bool VTL_content_platform_flags_CheckW(const VTL_content_platform_flags flags);

#ifdef __cplusplus
}
#endif


#endif