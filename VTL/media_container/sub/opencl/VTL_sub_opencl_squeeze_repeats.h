#ifndef VTL_SUB_OPENCL_SQUEEZE_REPEATS_H
#define VTL_SUB_OPENCL_SQUEEZE_REPEATS_H

#include <stddef.h>
#include <VTL/VTL_app_result.h>

#ifdef __cplusplus
extern "C" {
#endif

// Массовое удаление повторяющихся символов в строках через OpenCL
// in_texts: массив указателей на строки (вход)
// out_texts: массив новых строк (char*), выделяется внутри функции, освобождать вызывающему
// count: количество строк
// Возвращает 0 при успехе, иначе код ошибки
VTL_AppResult VTL_sub_OpenclSqueezeRepeats(const char** in_texts, char*** out_texts, size_t count);

#ifdef __cplusplus
}
#endif

#endif // VTL_SUB_OPENCL_SQUEEZE_REPEATS_H 