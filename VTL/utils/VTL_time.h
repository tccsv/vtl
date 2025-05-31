#pragma once

#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE

#include <sys/types.h>  // for time_t
#include <time.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef time_t VTL_Time;

VTL_Time VTL_time_GetCurrent(void);

#ifdef __cplusplus
}
#endif