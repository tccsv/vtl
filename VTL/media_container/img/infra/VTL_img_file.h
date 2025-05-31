#ifndef _VTL_IMG_FILE_H
#define _VTL_IMG_FILE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/media_container/img/VTL_img_data.h>

VTL_AppResult VTL_img_file_get_info(const char* filename, VTL_img_data_t* img_data);
VTL_AppResult VTL_img_file_validate(const char* filename);

#ifdef __cplusplus
}
#endif

#endif // _VTL_IMG_FILE_H 