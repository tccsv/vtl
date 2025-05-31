#ifndef _VTL_PUBLICATION_TEXT_DATA_H
#define _VTL_PUBLICATION_TEXT_DATA_H

#ifdef __cplusplus
extern "C"
{
#endif


#include <VTL/VTL_AppResult.h> 
#include <stddef.h>


typedef char VTL_publication_TextSymbol;

typedef struct _VTL_publication_Text
{
    VTL_publication_text_symbol* text;
    size_t length;
} VTL_publication_Text;


typedef enum _VTL_publication_marked_text_ModificationShift
{
    VTL_publication_text_type_kBold = 0,
    VTL_publication_text_type_kItalic,
    VTL_publication_text_type_kStrikethrough
} VTL_publication_marked_text_ModificationShift;

#define VTL_publication_text_ModificationBold 1 << 0
#define VTL_publication_text_ModificationItalic 1 << 1
#define VTL_publication_text_ModificationStrikethrough 1 << 2

typedef int VTL_publication_text_ModificationFlags;

typedef struct _VTL_publication_MarkedTextPart
{
    VTL_publication_TextSymbol* text;
    size_t length;
    VTL_publication_text_ModificationFlags type;
} VTL_publication_MarkedTextPart;


typedef struct _VTL_publication_MarkedTextBlock
{
    VTL_publication_marked_text_part* parts;
    size_t length;
} VTL_publication_MarkedTextBlock;

typedef VTL_publication_marked_text_block VTL_publication_MarkedText;

#define VTL_publication_text_default_length 100000
#define VTL_publication_text_default_size (VTL_publication_text_default_length*sizeof(VTL_publication_text_symbol)+sizeof(size_t))


void VTL_publication_marked_text_modification_SetBold(VTL_publication_text_ModificationFlags* p_flags);
void VTL_publication_marked_text_modification_SetItalic(VTL_publication_text_ModificationFlags* p_flags);
void VTL_publication_marked_text_modification_SetStrikethrough(VTL_publication_text_ModificationFlags* p_flags);


#ifdef __cplusplus
}
#endif


#endif
