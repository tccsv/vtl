#ifndef _VTL_CONVERTER_H
#define _VTL_CONVERTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <VTL/VTL_app_result.h>
#include <VTL/publication/text/VTL_publication_text_data.h>

/**
хедер для конвертера
*/

// структура с конфигурацией для конверсии
typedef struct {
    const char* input_file;
    const char* output_file;
    VTL_publication_marked_text_MarkupType input_type;
    VTL_publication_marked_text_MarkupType output_type;
} VTL_publication_ConversionConfig;

// функции конверсии
VTL_publication_app_result VTL_publication_convert_markup_s(const VTL_publication_ConversionConfig* config);
VTL_publication_app_result VTL_publication_std_md_to_tg_md_s(const char* std_md, size_t std_md_size, char** tg_md, size_t* tg_md_size);
VTL_publication_app_result VTL_publication_tg_md_to_std_md_s(const char* tg_md, size_t tg_md_size, char** std_md, size_t* std_md_size);
VTL_publication_app_result VTL_publication_md_to_plain_s(const char* md, size_t md_size, char** plain, size_t* plain_size);

#ifdef __cplusplus
}
#endif

#endif 