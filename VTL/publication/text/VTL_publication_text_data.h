#ifndef _VTL_PUBLICATION_TEXT_DATA_H
#define _VTL_PUBLICATION_TEXT_DATA_H

#ifdef __cplusplus
extern "C"
{
#endif


#include <VTL/VTL_app_result.h> 
#include <stddef.h>


typedef char VTL_publication_text_symbol;

typedef struct _VTL_publication_text
{
    VTL_publication_text_symbol* text;
    size_t length;
} VTL_publication_text;


typedef enum _VTL_publication_marked_text_modification_shift
{
    VTL_publication_text_type_kBold = 0,
    VTL_publication_text_type_kItalic,
    VTL_publication_text_type_kStrikethrough
} VTL_publication_marked_text_modification_shift;

#define VTL_publication_text_modification_bold 1 << 0
#define VTL_publication_text_modification_italic 1 << 1
#define VTL_publication_text_modification_strikethrough 1 << 2

typedef int VTL_publication_text_modification_flags;

typedef struct _VTL_publication_marked_text_part
{
    VTL_publication_text_symbol* text;
    size_t length;
    VTL_publication_text_modification_flags type;
} VTL_publication_marked_text_part;


typedef struct _VTL_publication_marked_text_block
{
    VTL_publication_marked_text_part* parts;
    size_t length;
} VTL_publication_marked_text_block;

typedef VTL_publication_marked_text_block VTL_publication_marked_text;

#define VTL_publication_text_default_length 100000
#define VTL_publication_text_default_size (VTL_publication_text_default_length*sizeof(VTL_publication_text_symbol)+sizeof(size_t))


void VTL_publication_marked_text_modification_SetBold(VTL_publication_text_modification_flags* p_flags);
void VTL_publication_marked_text_modification_SetItalic(VTL_publication_text_modification_flags* p_flags);
void VTL_publication_marked_text_modification_SetStrikethrough(VTL_publication_text_modification_flags* p_flags);


#ifdef __cplusplus
}
#endif


#endif