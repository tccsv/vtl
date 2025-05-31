#ifndef _VTL_PUBLICATION_TEXT_DATA_H
#define _VTL_PUBLICATION_TEXT_DATA_H

#ifdef __cplusplus
extern "C"
{
#endif


#include <VTL/VTL_publication_markup_text_flags.h>
#include <VTL/VTL_app_result.h> 
#include <stddef.h>


typedef char VTL_publication_text_Symbol;

typedef struct _VTL_publication_Text
{
    VTL_publication_text_Symbol* text;
    size_t length;
} VTL_publication_Text;

typedef enum _VTL_publication_marked_text_modification_Shift
{
    VTL_text_type_kBold = 0,
    VTL_text_type_kItalic,
    VTL_text_type_kStrikethrough
} VTL_publication_marked_text_modification_Shift;

#define VTL_TEXT_MODIFICATION_BOLD 1 << 0
#define VTL_TEXT_MODIFICATION_ITALIC 1 << 1
#define VTL_TEXT_MODIFICATION_STRIKETHROUGH 1 << 2

typedef int VTL_publication_text_modification_Flags;

typedef struct _VTL_publication_marked_text_Part
{
    VTL_publication_text_Symbol* text;
    size_t length;
    VTL_publication_text_modification_Flags type;
} VTL_publication_marked_text_Part;


typedef struct _VTL_publication_marked_text_Block
{
    VTL_publication_marked_text_Part* parts;
    size_t length;
} VTL_publication_marked_text_Block;

typedef VTL_publication_marked_text_Block VTL_publication_MarkedText;

#define VTL_PUBLICATION_TEXT_DEFAULT_LENGTH 100000
#define VTL_PUBLICATION_TEXT_DEFAULT_SIZE (VTL_PUBLICATION_TEXT_DEFAULT_LENGTH*sizeof(VTL_text_Symbol)+sizeof(size_t))


void VTL_publication_marked_text_modification_SetBold(VTL_publication_text_modification_Flags* p_flags);
void VTL_publication_marked_text_modification_SetItalic(VTL_publication_text_modification_Flags* p_flags);
void VTL_publication_marked_text_modification_SetStrikethrough(VTL_publication_text_modification_Flags* p_flags);


#ifdef __cplusplus
}
#endif


#endif