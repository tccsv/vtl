#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE
#include <time.h>
#include <stddef.h>
#include "VTL_time.h"

time_t time(time_t *t);

VTL_Time VTL_time_GetCurrent(void)
{
    time_t current_time;
    return time(&current_time);
}