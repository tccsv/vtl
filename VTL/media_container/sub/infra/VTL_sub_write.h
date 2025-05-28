#ifndef VTL_SUB_WRITE_H
#define VTL_SUB_WRITE_H

#include <VTL/media_container/sub/VTL_sub_data.h>
#include <VTL/media_container/sub/VTL_sub_style.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct VTL_sub_WriteSink VTL_sub_WriteSink;

typedef struct VTL_sub_WriteMeta {
    VTL_sub_Format format;
    // Можно добавить язык, кодировку и др. при необходимости
} VTL_sub_WriteMeta;

// Открыть приёмник субтитров для записи
VTL_AppResult VTL_sub_WriteOpenSink(const char* file_path, VTL_sub_Format format, VTL_sub_WriteSink** pp_sink, const VTL_sub_StyleParams* style_params);
// Записать одну запись (реплику) субтитров
VTL_AppResult VTL_sub_WritePart(VTL_sub_WriteSink* p_sink, const VTL_sub_Entry* p_entry);
// Закрыть приёмник
VTL_AppResult VTL_sub_WriteCloseSink(VTL_sub_WriteSink** pp_sink);

#ifdef __cplusplus
}
#endif

#endif // VTL_SUB_WRITE_H 