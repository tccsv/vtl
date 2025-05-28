#ifndef VTL_SUB_OPENCL_FORMAT_NUMBERS_H
#define VTL_SUB_OPENCL_FORMAT_NUMBERS_H

#include <stddef.h>
#include <VTL/VTL_app_result.h>

#ifdef __cplusplus
extern "C" {
#endif

// Массовое форматирование чисел в строках через OpenCL
// in_texts: массив указателей на строки (вход)
// out_texts: массив новых строк (char*), выделяется внутри функции, освобождать вызывающему
// count: количество строк
// sep: символ-разделитель групп (например, ',' или ' ')
// group_len: длина группы (обычно 3)
// Возвращает 0 при успехе, иначе код ошибки
VTL_AppResult VTL_sub_OpenclFormatNumbers(const char** in_texts, char*** out_texts, size_t count, char sep, int group_len);

#ifdef __cplusplus
}
#endif

#endif // VTL_SUB_OPENCL_FORMAT_NUMBERS_H 