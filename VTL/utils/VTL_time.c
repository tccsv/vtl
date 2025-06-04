#include <VTL/utils/VTL_time.h>


VTL_Time VTL_time_GetCurrent(void)
{
    return time(NULL);
}