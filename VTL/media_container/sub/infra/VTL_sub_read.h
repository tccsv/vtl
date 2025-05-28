#ifndef VTL_SUB_READ_H
#define VTL_SUB_READ_H

#include <VTL/media_container/sub/VTL_sub_data.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct VTL_sub_ReadSource VTL_sub_ReadSource;

typedef struct VTL_sub_ReadMeta {
    VTL_sub_Format format;
    size_t entry_count;
    // Можно добавить язык, кодировку и др. при необходимости
} VTL_sub_ReadMeta;

// Открыть источник субтитров для чтения
VTL_AppResult VTL_sub_ReadOpenSource(const char* file_path, VTL_sub_ReadSource** pp_source);
// Закрыть источник
VTL_AppResult VTL_sub_ReadCloseSource(VTL_sub_ReadSource** pp_source);
// Прочитать метаданные (формат, количество реплик и т.д.)
VTL_AppResult VTL_sub_ReadMetaData(VTL_sub_ReadSource* p_source, VTL_sub_ReadMeta* p_meta);
// Прочитать одну запись (реплику) субтитров (итеративно)
VTL_AppResult VTL_sub_ReadPart(VTL_sub_ReadSource* p_source, VTL_sub_Entry* p_entry);

#ifdef __cplusplus
}
#endif

#endif // VTL_SUB_READ_H 