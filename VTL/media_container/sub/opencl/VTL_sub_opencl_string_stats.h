#ifndef VTL_SUB_OPENCL_STRING_STATS_H
#define VTL_SUB_OPENCL_STRING_STATS_H

#include <stddef.h>
#include <stdint.h>
#include <VTL/VTL_app_result.h>

#ifdef __cplusplus
extern "C" {
#endif

// Структура для хранения статистики по строке
typedef struct {
    uint32_t length;              // длина строки
    uint32_t word_count;          // количество слов
    uint32_t unique_chars;        // количество уникальных символов (ASCII)
    uint32_t char_freq[128];      // частота встречаемости ASCII-символов (0..127)
} VTL_StringStats;

// Массовое вычисление статистики по строкам через OpenCL
// in_texts: массив указателей на строки (вход)
// out_stats: массив структур VTL_StringStats (выделяется внутри функции, освобождать вызывающему)
// count: количество строк
// Возвращает 0 при успехе, иначе код ошибки
VTL_AppResult VTL_sub_OpenclStringStats(const char** in_texts, VTL_StringStats** out_stats, size_t count);

#ifdef __cplusplus
}
#endif

#endif // VTL_SUB_OPENCL_STRING_STATS_H 