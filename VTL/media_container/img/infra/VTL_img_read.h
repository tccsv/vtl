#ifndef _VTL_IMG_READ_H
#define _VTL_IMG_READ_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/media_container/img/VTL_img_data.h>

VTL_AppResult VTL_img_read_from_file(const char* filename, VTL_img_data_t* img_data);

#ifdef __cplusplus
}
#endif

#endif // _VTL_IMG_READ_H 