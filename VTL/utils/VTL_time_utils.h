#ifndef VTL_TIME_UTILS_H
#define VTL_TIME_UTILS_H

#include <time.h>
#include <string.h>
#include <VTL/utils/VTL_time.h>


void VTL_time_to_sql(const VTL_Time* p_time, char* buf, size_t buf_size);

#endif