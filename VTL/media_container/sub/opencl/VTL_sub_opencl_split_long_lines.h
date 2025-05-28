#ifndef VTL_SUB_OPENCL_SPLIT_LONG_LINES_H
#define VTL_SUB_OPENCL_SPLIT_LONG_LINES_H

#include <stddef.h>
#include <VTL/VTL_app_result.h>

#ifdef __cplusplus
extern "C" {
#endif

// Разделение длинных строк на несколько по максимальной длине (по словам)
// in_texts: массив указателей на строки (вход)
// out_texts: массив массивов строк (char***) — для каждой входной строки массив новых строк, выделяется внутри функции, освобождать вызывающему
// out_counts: массив количества строк для каждого in_texts[i]
// count: количество входных строк
// max_len: максимальная длина строки
// Возвращает 0 при успехе, иначе код ошибки
VTL_AppResult VTL_sub_OpenclSplitLongLines(const char** in_texts, char**** out_texts, int** out_counts, size_t count, int max_len);

#ifdef __cplusplus
}
#endif

#endif // VTL_SUB_OPENCL_SPLIT_LONG_LINES_H 