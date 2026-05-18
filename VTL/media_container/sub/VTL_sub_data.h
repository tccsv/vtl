#ifndef _VTL_SUB_DATA_H
#define _VTL_SUB_DATA_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/utils/VTL_buffer_data.h>
#include <VTL/utils/VTL_string.h>
#include <stdint.h>
#include <stddef.h>

typedef enum _VTL_sub_Format
{
    VTL_sub_format_kASS = 0,
    VTL_sub_format_kSRT,
    VTL_sub_format_kVTT,
    VTL_sub_format_kTTML
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

typedef struct _VTL_sub_color_RGB
{
    VTL_sub_ColorPart r;
    VTL_sub_ColorPart g;
    VTL_sub_ColorPart b;
} VTL_sub_color_RGB;

typedef struct _VTL_sub_Params
{
    VTL_sub_HorizontalAlign horizontal_align;
    VTL_sub_color_RGB color;    
    VTL_sub_color_RGB background_color;    
    VTL_sub_color_RGB outline_color;
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

typedef struct _VTL_Sub
{
    VTL_sub_MetaData meta_data;
    VTL_BufferData* data;
    size_t length;
} VTL_Sub;


#ifdef __cplusplus
}
#endif


#endif