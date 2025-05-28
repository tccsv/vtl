#ifndef VTL_SUB_OPENCL_H
#define VTL_SUB_OPENCL_H

#include <stddef.h>
#include <VTL/VTL_app_result.h>

#ifdef __cplusplus
extern "C" {
#endif

// Массовая обработка массива строк субтитров через OpenCL (например, удаление тегов)
// in_texts: массив указателей на строки (вход)
// out_texts: массив указателей на строки (выход, выделяется внутри функции, освобождать вызывающему)
// count: количество строк
// Возвращает 0 при успехе, иначе код ошибки
VTL_AppResult VTL_sub_OpenclStripTags(const char** in_texts, char** out_texts, size_t count);

#ifdef __cplusplus
}
#endif

#endif // VTL_SUB_OPENCL_H 