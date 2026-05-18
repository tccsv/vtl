#include <VTL/content_platform/VTL_content_platform_flags_check.h>

bool VTL_content_platform_flags_CheckTg(const VTL_content_platform_flags flags)
{
    return flags & VTL_CONTENT_PLATFORM_TG == VTL_CONTENT_PLATFORM_TG;
}

bool VTL_content_platform_flags_CheckW(const VTL_content_platform_flags flags)
{
    return flags & VTL_CONTENT_PLATFORM_W == VTL_CONTENT_PLATFORM_W;
}
