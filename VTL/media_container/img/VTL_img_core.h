#ifndef _VTL_IMG_CORE_H
#define _VTL_IMG_CORE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "VTL_img_data.h"

VTL_ImageContext* VTL_img_context_Init(void);
void VTL_img_context_Cleanup(VTL_ImageContext* ctx);

int VTL_img_LoadImage(const char* input_path, VTL_ImageContext* ctx);
int VTL_img_SaveImage(const char* output_path, VTL_ImageContext* ctx);
int VTL_img_ApplyFilter(VTL_ImageContext* ctx, const VTL_ImageFilter* filter);

int VTL_img_InitGPU(void);
void VTL_img_CleanupGPU(void);

#ifdef __cplusplus
}
#endif

#endif // _VTL_IMG_CORE_H 