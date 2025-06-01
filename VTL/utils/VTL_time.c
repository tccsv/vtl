#include <VTL/utils/VTL_time.h>

VTL_time VTL_time_GetCurrent(void)
{
    return time(NULL);
}
