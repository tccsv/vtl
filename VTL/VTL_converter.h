#ifndef VTL_CONVERTER_H
#define VTL_CONVERTER_H

#include "VTL_app_result.h"
#include "VTL_markup_types.h"
#include <stddef.h>

/**
хедер для конвертера
*/

// структура с конфигурацией для конверсии
typedef struct {
    const char* input_file;
    const char* output_file;
    VTL_MarkupType input_type;
    VTL_MarkupType output_type;
} VTL_ConversionConfig;

// функции конверсии
VTL_AppResult VTL_convert_markup_s(const VTL_ConversionConfig* config);
VTL_AppResult VTL_convert_to_internal_s(const char* content, size_t size, VTL_MarkupType from_type, char** internal_content, size_t* internal_size);
VTL_AppResult VTL_convert_from_internal_s(const char* internal_content, size_t internal_size, VTL_MarkupType to_type, char** output_content, size_t* output_size);

// конкретные функции конверсии
VTL_AppResult VTL_std_md_to_tg_md_s(const char* std_md, size_t std_md_size, char** tg_md, size_t* tg_md_size);
VTL_AppResult VTL_tg_md_to_std_md_s(const char* tg_md, size_t tg_md_size, char** std_md, size_t* std_md_size);
VTL_AppResult VTL_md_to_plain_s(const char* md, size_t md_size, char** plain, size_t* plain_size);

#endif 