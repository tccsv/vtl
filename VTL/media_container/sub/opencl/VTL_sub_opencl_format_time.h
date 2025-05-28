#ifndef VTL_SUB_OPENCL_FORMAT_TIME_H
#define VTL_SUB_OPENCL_FORMAT_TIME_H

#include <stddef.h>
#include <VTL/VTL_app_result.h>

#ifdef __cplusplus
extern "C" {
#endif

// Массовое форматирование временных меток (секунды -> строка SRT/ASS/VTT) через OpenCL
// in_times: массив секунд (double)
// out_texts: массив строк (char*), выделяется внутри функции, освобождать вызывающему
// count: количество временных меток
// format: 0 = SRT (00:00:00,000), 1 = ASS (0:00:00.00), 2 = VTT (00:00:00.000)
// Возвращает 0 при успехе, иначе код ошибки
VTL_AppResult VTL_sub_OpenclFormatTime(const double* in_times, char*** out_texts, size_t count, int format);

#ifdef __cplusplus
}
#endif

#endif // VTL_SUB_OPENCL_FORMAT_TIME_H 