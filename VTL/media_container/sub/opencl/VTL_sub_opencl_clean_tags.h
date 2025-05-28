#ifndef VTL_SUB_OPENCL_CLEAN_TAGS_H
#define VTL_SUB_OPENCL_CLEAN_TAGS_H

#include <stddef.h>
#include <VTL/VTL_app_result.h>

#ifdef __cplusplus
extern "C" {
#endif

// Массовая очистка строк от тегов, спецсимволов и эмодзи с помощью OpenCL
VTL_AppResult VTL_sub_OpenclCleanTags(const char** in_texts, char** out_texts, size_t count);

#ifdef __cplusplus
}
#endif

#endif // VTL_SUB_OPENCL_CLEAN_TAGS_H 