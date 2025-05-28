#ifndef _VTL_SUB_PARSE_H
#define _VTL_SUB_PARSE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <VTL/VTL_app_result.h>
#include <VTL/media_container/sub/VTL_sub_data.h> // Для VTL_sub_Format, VTL_sub_List

// Определяет формат субтитров по расширению файла
VTL_AppResult VTL_sub_ParseDetectFormat(const char* filename, VTL_sub_Format* out_format);

// Парсит файл субтитров в структуру VTL_sub_List
VTL_AppResult VTL_sub_ParseFile(const char* input_file, VTL_sub_Format input_format, VTL_sub_List* out_list);

// Освобождает память, выделенную для VTL_sub_List и его элементов
VTL_AppResult VTL_sub_ListFree(VTL_sub_List* list);

#ifdef __cplusplus
}
#endif

#endif // _VTL_SUB_PARSE_H 