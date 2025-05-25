#ifndef _VTL_PUBLICATION_MARKUP_TEXT_FLAGS_H
#define _VTL_PUBLICATION_MARKUP_TEXT_FLAGS_H

#ifdef __cplusplus
extern "C"
{
#endif


#include <VTL/utils/VTL_file.h>
#include <stdbool.h>


typedef enum _VTL_publication_marked_text_markup_type
{
    VTL_publication_markup_type_kStandardMD = 0,
    VTL_publication_markup_type_kTelegramMD,
    VTL_publication_markup_type_kHTML,
    VTL_publication_markup_type_kBB
} VTL_publication_marked_text_markup_type;

#define VTL_publication_marked_text_markup_type_max VTL_publication_markup_type_kBB
#define VTL_publication_text_type_regular_shift (VTL_publication_marked_text_markup_type_max + 1)
#define VTL_publication_text_type_max VTL_publication_text_type_regular_shift  

typedef int VTL_publication_marked_text_type_flags; 

#define VTL_publication_text_type_standard_md (1 << VTL_publication_markup_type_kStandardMD)
#define VTL_publication_text_type_telegram_md (1 << VTL_publication_markup_type_kTelegramMD)
#define VTL_publication_text_type_html (1 << VTL_publication_markup_type_kHTML)
#define VTL_publication_text_type_bb (1 << VTL_publication_markup_type_kBB)
#define VTL_publication_text_type_regular (1 << VTL_publication_text_type_regular_shift)

bool VTL_publication_marked_text_type_flag_CheckStandardMD(const VTL_publication_marked_text_type_flags flags);
bool VTL_publication_marked_text_type_flag_CheckTelegramMD(const VTL_publication_marked_text_type_flags flags);
bool VTL_publication_marked_text_type_flag_CheckHTML(const VTL_publication_marked_text_type_flags flags);
bool VTL_publication_marked_text_type_flag_CheckBB(const VTL_publication_marked_text_type_flags flags);
bool VTL_publication_marked_text_type_flag_CheckRegularText(const VTL_publication_marked_text_type_flags flags);



#ifdef __cplusplus
}
#endif


#endif