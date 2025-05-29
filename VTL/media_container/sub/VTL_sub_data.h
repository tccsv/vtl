#ifndef _VTL_SUB_DATA_H
#define _VTL_SUB_DATA_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/utils/VTL_buffer_data.h>
#include <VTL/utils/VTL_string.h>
#include <VTL/VTL_app_result.h>
#include <stdint.h>
#include <stddef.h>
#include <VTL/media_container/sub/VTL_sub_style.h>

typedef enum _VTL_sub_Format
{
    VTL_sub_format_kASS = 0,
    VTL_sub_format_kSRT,
    VTL_sub_format_kVTT,
    VTL_sub_format_kUNKNOWN
} VTL_sub_Format;

typedef enum _VTL_sub_HorizontalAlign
{
    VTL_sub_horizontal_align_kLeft = 0,
    VTL_sub_format_kCenter,
    VTL_sub_format_kRight
} VTL_sub_HorizontalAlign;

typedef VTL_StandartString VTL_sub_FontName;
typedef uint_least8_t VTL_sub_Size;
typedef uint_least8_t VTL_sub_ColorPart;

typedef struct VTL_sub_ColorRGB {
    VTL_sub_ColorPart r;
    VTL_sub_ColorPart g;
    VTL_sub_ColorPart b;
} VTL_sub_ColorRGB;

typedef struct _VTL_sub_Params
{
    VTL_sub_HorizontalAlign horizontal_align;
    VTL_sub_ColorRGB color;    
    VTL_sub_ColorRGB background_color;    
    VTL_sub_ColorRGB outline_color;
    VTL_sub_Size text_size;
    VTL_sub_Size margin;
    VTL_sub_FontName font_name;
} VTL_sub_Params;

typedef struct _VTL_sub_MetaData
{
    size_t data_size;    
    VTL_sub_Params params;
} VTL_sub_MetaData;

typedef char VTL_sub_Symbol;
typedef struct _VTL_sub_Str
{
    VTL_sub_Symbol* p_data;
    size_t length;
} VTL_sub_Str;

typedef struct _VTL_sub_Subtitles
{
    VTL_sub_MetaData meta_data;
    VTL_BufferData* data;
    size_t length;
} VTL_sub_Subtitles;

// Универсальное представление одного субтитра (для любого формата)
typedef struct VTL_sub_Entry {
    int index;    // номер субтитра (для SRT обязателен, для VTT/ASS может быть идентификатором или отсутствовать)
    double start; // время начала отображения субтитра в секундах
    double end;   // время конца отображения субтитра в секундах
    char* text;   // текст субтитра (для ASS - очищенный от тегов)
    char* style;  // информация о стиле (для ASS - имя стиля, для VTT - идентификатор реплики, для SRT - обычно NULL)
} VTL_sub_Entry;

// Массив записей субтитров (универсальный для SRT/ASS/VTT)
typedef struct VTL_sub_List {
    VTL_sub_Entry* entries;
    size_t count;
} VTL_sub_List;

// Функции для VTL_sub_Params
VTL_AppResult VTL_sub_ParamsSetHorizontalAlign(VTL_sub_Params* p_params, VTL_sub_HorizontalAlign align);
VTL_AppResult VTL_sub_ParamsSetColor(VTL_sub_Params* p_params, VTL_sub_ColorRGB color);
VTL_AppResult VTL_sub_ParamsSetBackgroundColor(VTL_sub_Params* p_params, VTL_sub_ColorRGB background_color);
VTL_AppResult VTL_sub_ParamsSetOutlineColor(VTL_sub_Params* p_params, VTL_sub_ColorRGB outline_color);
VTL_AppResult VTL_sub_ParamsSetTextSize(VTL_sub_Params* p_params, VTL_sub_Size text_size);
VTL_AppResult VTL_sub_ParamsSetMargin(VTL_sub_Params* p_params, VTL_sub_Size margin);
VTL_AppResult VTL_sub_ParamsSetFontName(VTL_sub_Params* p_params, VTL_sub_FontName font_name);

// Функции для VTL_sub_List
VTL_AppResult VTL_sub_ListCreate(VTL_sub_List** pp_sub_list);
VTL_AppResult VTL_sub_ListDestroy(VTL_sub_List** pp_sub_list);
VTL_AppResult VTL_sub_ListAddEntry(VTL_sub_List* p_sub_list, const VTL_sub_Entry* p_entry);
VTL_AppResult VTL_sub_ListGetEntry(const VTL_sub_List* p_sub_list, size_t index, VTL_sub_Entry** pp_entry);
VTL_AppResult VTL_sub_ListRemoveEntry(VTL_sub_List* p_sub_list, size_t index);

// Функции для парсинга и форматирования субтитров
VTL_AppResult VTL_sub_Parse(VTL_BufferData* p_buffer_data, VTL_sub_Format format, VTL_sub_List** pp_sub_list);
VTL_AppResult VTL_sub_ParseSrt(VTL_BufferData* p_buffer_data, VTL_sub_Format format, VTL_sub_List** pp_sub_list);
VTL_AppResult VTL_sub_FormatToString(const VTL_sub_List* p_sub_list, VTL_sub_Format format, VTL_BufferData** pp_buffer_data, const VTL_sub_StyleParams* style_params);

// Функции для загрузки и сохранения списка субтитров из/в файл
// Пытается определить формат файла по его расширению. 
// Если file_path равен NULL или не удалось определить формат, возвращает ошибку.
// detected_format (выходной) - сюда будет записан определенный формат файла.
VTL_AppResult VTL_sub_LoadFromFile(const char* file_path, VTL_sub_Format* detected_format_out, VTL_sub_List** pp_sub_list);

// Сохраняет список субтитров в файл в указанном формате.
// Если format равен VTL_sub_format_kUNKNOWN, пытается определить формат по расширению file_path.
// Если p_sub_list равен NULL или file_path равен NULL, возвращает ошибку.
VTL_AppResult VTL_sub_SaveToFile(const char* file_path, VTL_sub_Format format, const VTL_sub_List* p_sub_list, const VTL_sub_StyleParams* style_params);

VTL_AppResult VTL_sub_ParseSrtAllocBuffer(const VTL_BufferData* p_buffer_data, char** buffer_copy_out);
VTL_AppResult VTL_sub_ParseSrtProcessBlock(VTL_sub_Entry* current_entry, char* text_buffer, VTL_sub_Format format, int* auto_increment_index, VTL_sub_List** pp_sub_list);
void VTL_sub_ParseSrtResetEntry(VTL_sub_Entry* entry, char* text_buffer);
VTL_AppResult VTL_sub_ParseSrtHandleEmptyLine(int* state, VTL_sub_Entry* current_entry, char* text_buffer, VTL_sub_Format format, int* auto_increment_index, VTL_sub_List** pp_sub_list);
VTL_AppResult VTL_sub_ParseSrtHandleNewIndexTime(int* state, int is_new_index, int is_new_time, VTL_sub_Entry* current_entry, char* text_buffer, VTL_sub_Format format, int* auto_increment_index, VTL_sub_List** pp_sub_list);
VTL_AppResult VTL_sub_ParseSrtHandleTimeLine(VTL_sub_Entry* current_entry, char* time_start_str, char* time_end_str, VTL_sub_Format format, int* state, int* auto_increment_index, char* text_buffer);
VTL_AppResult VTL_sub_ParseSrtHandleVttId(VTL_sub_Entry* current_entry, const char* line, int* state);
VTL_AppResult VTL_sub_ParseSrtAddText(char* text_buffer, const char* line);

#ifdef __cplusplus
}
#endif


#endif