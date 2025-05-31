#ifndef _VTL_BUFFER_DATA_H
#define _VTL_BUFFER_DATA_H

#ifdef __cplusplus
extern "C"
{
#endif


#include <stddef.h>

typedef struct _VTL_BufferData
{
    char* data;
    size_t data_size;
} VTL_BufferData;

#define VTL_BUFFER_DATA_LENGTH 1024u * 10

#define VTL_BUFFER_DATA_SIZE_DEFAULT (sizeof(char) * VTL_BUFFER_DATA_LENGTH)

#ifdef __cplusplus
}
#endif


#endif