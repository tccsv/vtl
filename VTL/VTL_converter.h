#ifndef _VTL_CONVERTER_H
#define _VTL_CONVERTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <VTL/VTL_app_result.h>
#include <VTL/publication/text/VTL_publication_text_data.h>
#include <VTL/VTL_publication_markup_text_flags.h>

/**
хедер для конвертера
*/

// структура с конфигурацией для конверсии
typedef struct _VTL_publication_ConversionConfig {
    const char* input_file;
    const char* output_file;
    VTL_publication_marked_text_MarkupType input_type;
    VTL_publication_marked_text_MarkupType output_type;
} VTL_publication_ConversionConfig;

// функции конверсии
VTL_publication_app_result VTL_publication_ConvertMarkupS(const VTL_publication_ConversionConfig* config);
VTL_publication_app_result VTL_publication_StdMdToTgMdS(const char* std_md, size_t std_md_size, char** tg_md, size_t* tg_md_size);
VTL_publication_app_result VTL_publication_TgMdToStdMdS(const char* tg_md, size_t tg_md_size, char** std_md, size_t* std_md_size);
VTL_publication_app_result VTL_publication_MdToPlainS(const char* md, size_t md_size, char** plain, size_t* plain_size);

#ifdef __cplusplus
}
#endif

#endif 