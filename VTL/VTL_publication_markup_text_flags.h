#ifndef _VTL_PUBLICATION_MARKUP_TEXT_FLAGS_H
#define _VTL_PUBLICATION_MARKUP_TEXT_FLAGS_H

#ifdef __cplusplus
extern "C"
{
#endif


#include <VTL/utils/VTL_file.h>
#include <stdbool.h>


typedef enum _VTL_publication_marked_text_MarkupType
{
    VTL_markup_type_kStandartMD = 0,
    VTL_markup_type_kTelegramMD,
    VTL_markup_type_kHTML,
    VTL_markup_type_kBB
} VTL_publication_marked_text_MarkupType;

#define VTL_PUBLICATION_MARKED_TEXT_MARKUP_TYPE_MAX VTL_markup_type_kBB
#define VTL_PUBLICATION_TEXT_TYPE_REGULAR_SHIFT (VTL_PUBLICATION_MARKED_TEXT_MARKUP_TYPE_MAX + 1)
#define VTL_PUBLICATION_TEXT_TYPE_MAX VTL_PUBLICATION_TEXT_TYPE_REGULAR_SHIFT  

typedef int VTL_publication_marked_text_type_Flags; 

#define VTL_PUBLICATION_TEXT_TYPE_STANDART_MD (1 << VTL_markup_type_kStandartMD)
#define VTL_PUBLICATION_TEXT_TYPE_TELEGRAM_MD (1 << VTL_markup_type_kTelegramMD)
#define VTL_PUBLICATION_TEXT_TYPE_HTML (1 << VTL_markup_type_kHTML)
#define VTL_PUBLICATION_TEXT_TYPE_BB (1 << VTL_markup_type_kBB)
#define VTL_PUBLICATION_TEXT_TYPE_REGULAR (1 << VTL_PUBLICATION_TEXT_TYPE_REGULAR_SHIFT)

bool VTL_publication_marked_text_type_flag_CheckStandartMD(const VTL_publication_marked_text_type_Flags flags);
bool VTL_publication_marked_text_type_flag_CheckTelegramMD(const VTL_publication_marked_text_type_Flags flags);
bool VTL_publication_marked_text_type_flag_CheckHTML(const VTL_publication_marked_text_type_Flags flags);
bool VTL_publication_marked_text_type_flag_CheckBB(const VTL_publication_marked_text_type_Flags flags);
bool VTL_publication_marked_text_type_flag_CheckRegularText(const VTL_publication_marked_text_type_Flags flags);



#ifdef __cplusplus
}
#endif


#endif