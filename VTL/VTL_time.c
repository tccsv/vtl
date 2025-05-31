#include <VTL/utils/VTL_time.h>

typedef struct {
    time_t unix_time;
    struct tm local_time;
    uint64_t milliseconds;
} VTL_Time;

VTL_Time VTL_time_GetCurrent(void)
{
    VTL_Time result;
    struct timespec ts;
    
    clock_gettime(CLOCK_REALTIME, &ts);
    result.unix_time = ts.tv_sec;
    result.milliseconds = ts.tv_nsec / 1000000;
    
    localtime_r(&result.unix_time, &result.local_time);
    
    return result;
}