#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif
#endif
#include <time.h>
#include <stddef.h>
#include <VTL/utils/VTL_utils_time.h>

time_t time(time_t *t);

VTL_Time VTL_time_GetCurrent(void)
{
    time_t current_time;
    return time(&current_time);
}