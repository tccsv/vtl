#ifndef VTL_SUB_OPENCL_DETECT_ENCODING_H
#define VTL_SUB_OPENCL_DETECT_ENCODING_H

#include <stddef.h>
#include <VTL/VTL_app_result.h>

#ifdef __cplusplus
extern "C" {
#endif

// Поддерживаемые кодировки
// 0 = UTF-8, 1 = Windows-1251, 2 = Latin1, 3 = ASCII, 4 = неизвестно
// in_texts: массив указателей на строки (вход)
// out_encodings: массив int (выделяется внутри функции, освобождать вызывающему)
// count: количество строк
// Возвращает 0 при успехе, иначе код ошибки
VTL_AppResult VTL_sub_OpenclDetectEncoding(const char** in_texts, int** out_encodings, size_t count);

#ifdef __cplusplus
}
#endif

#endif // VTL_SUB_OPENCL_DETECT_ENCODING_H 