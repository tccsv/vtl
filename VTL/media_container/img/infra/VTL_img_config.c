#include "VTL_img_config.h"
#include <stdlib.h>
#include <string.h>

VTL_AppResult VTL_img_config_init(VTL_img_config_t* config)
{
    if (!config) {
        return VTL_IMG_ERROR_INVALID_PARAM;
    }

    memset(config, 0, sizeof(VTL_img_config_t));
    return VTL_img_config_set_defaults(config);
}

VTL_AppResult VTL_img_config_set_defaults(VTL_img_config_t* config)
{
    if (!config) {
        return VTL_IMG_ERROR_INVALID_PARAM;
    }

    config->quality = 90;
    config->max_width = 1920;
    config->max_height = 1080;
    config->format = AV_PIX_FMT_RGB24;

    return VTL_IMG_SUCCESS;
} 