#ifndef _VTL_IMG_FILTERS_H
#define _VTL_IMG_FILTERS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "VTL_img_data.h"

extern const VTL_ImageFilter VTL_img_filter_blur;
extern const VTL_ImageFilter VTL_img_filter_gaussian_blur;
extern const VTL_ImageFilter VTL_img_filter_sharpen;
extern const VTL_ImageFilter VTL_img_filter_edge;
extern const VTL_ImageFilter VTL_img_filter_sepia;
extern const VTL_ImageFilter VTL_img_filter_custom_conv;
extern const VTL_ImageFilter VTL_img_filter_grayscale;
extern const VTL_ImageFilter VTL_img_filter_rotate_90;
extern const VTL_ImageFilter VTL_img_filter_rotate_180;
extern const VTL_ImageFilter VTL_img_filter_rotate_270;
extern const VTL_ImageFilter VTL_img_filter_edge_sobel;
extern const VTL_ImageFilter VTL_img_filter_edge_prewitt;
extern const VTL_ImageFilter VTL_img_filter_edge_roberts;
extern const VTL_ImageFilter VTL_img_filter_edge_laplace;

const VTL_ImageFilter** VTL_img_GetAvailableFilters(void);

#ifdef __cplusplus
}
#endif

#endif // _VTL_IMG_FILTERS_H 