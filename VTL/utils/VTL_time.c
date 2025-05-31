#include <VTL/utils/VTL_time.h>

VTL_publication_time VTL_publication_time_GetCurrent(void)
{
    return time(NULL);
}