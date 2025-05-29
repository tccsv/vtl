#ifndef _VTL_IMG_CONFIG_H
#define _VTL_IMG_CONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/media_container/img/VTL_img_data.h>

typedef struct {
    int quality;
    int max_width;
    int max_height;
    int format;
} VTL_img_config_t;

VTL_AppResult VTL_img_config_init(VTL_img_config_t* config);
VTL_AppResult VTL_img_config_set_defaults(VTL_img_config_t* config);

#ifdef __cplusplus
}
#endif

#endif // _VTL_IMG_CONFIG_H 