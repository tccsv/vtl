#ifndef _VTL_IMG_CORE_H
#define _VTL_IMG_CORE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/media_container/img/VTL_img_data.h>
#include <VTL/VTL_app_result.h>

VTL_ImageContext* VTL_img_context_Init(void);
void VTL_img_context_Cleanup(VTL_ImageContext* ctx);

VTL_AppResult VTL_img_LoadImage(const char* input_path, VTL_ImageContext* ctx);
VTL_AppResult VTL_img_SaveImage(const char* output_path, VTL_ImageContext* ctx);
VTL_AppResult VTL_img_ApplyFilter(VTL_ImageContext* ctx, const VTL_ImageFilter* filter);

// Обрабатывает группу изображений параллельно
VTL_AppResult VTL_img_ProcessBatch(const char** input_paths, const VTL_ImageFilter** filters, const char** output_paths, int count);

VTL_AppResult VTL_img_InitGPU(void);
void VTL_img_CleanupGPU(void);

#ifdef __cplusplus
}
#endif

#endif // _VTL_IMG_CORE_H 