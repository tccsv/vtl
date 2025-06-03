#ifndef _VTL_IMG_WRITE_H
#define _VTL_IMG_WRITE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/media_container/img/VTL_img_data.h>

VTL_AppResult VTL_img_write_to_file(const char* filename, const VTL_img_data_t* img_data);

#ifdef __cplusplus
}
#endif

#endif // _VTL_IMG_WRITE_H 