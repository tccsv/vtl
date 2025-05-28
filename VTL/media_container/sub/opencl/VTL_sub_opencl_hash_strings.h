#ifndef VTL_SUB_OPENCL_HASH_STRINGS_H
#define VTL_SUB_OPENCL_HASH_STRINGS_H

#include <stddef.h>
#include <stdint.h>
#include <VTL/VTL_app_result.h>

#ifdef __cplusplus
extern "C" {
#endif

// Массовое вычисление CRC32-хешей для массива строк через OpenCL
// in_texts: массив указателей на строки (вход)
// out_hashes: массив uint32_t (выделяется внутри функции, освобождать вызывающему)
// count: количество строк
// Возвращает 0 при успехе, иначе код ошибки
VTL_AppResult VTL_sub_OpenclHashStrings(const char** in_texts, uint32_t** out_hashes, size_t count);

#ifdef __cplusplus
}
#endif

#endif // VTL_SUB_OPENCL_HASH_STRINGS_H 