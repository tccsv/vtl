#ifndef _VTL_TIME_H
#define _VTL_TIME_H

#ifdef __cplusplus
extern "C"
{
#endif


#include <time.h>

typedef time_t VTL_publication_time;

VTL_publication_time VTL_publication_time_GetCurrent(void);

#ifdef __cplusplus
}
#endif


#endif