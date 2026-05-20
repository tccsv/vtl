#include <VTL/utils/VTL_time_utils.h>
#include <stdio.h>

void VTL_time_to_sql(const VTL_Time *p_time, char *buf, size_t buf_size)
{
    time_t raw = (time_t)(*p_time);
    struct tm *tm_info = localtime(&raw);
    strftime(buf, buf_size, "%Y-%m-%d %H:%M:%S", tm_info);
}
