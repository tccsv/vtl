#ifndef _VTL_PUBLICATION_MARKUP_TEXT_FLAGS_H
#define _VTL_PUBLICATION_MARKUP_TEXT_FLAGS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

typedef enum _VTL_publication_marked_text_MarkupType
{
    VTL_publication_markup_type_kStandardMD = 0,
    VTL_publication_markup_type_kTelegramMD,
    VTL_publication_markup_type_kHTML,
    VTL_publication_markup_type_kBB
} VTL_publication_marked_text_MarkupType;

#define VTL_publication_marked_text_MarkupTypeMax VTL_publication_markup_type_kBB
#define VTL_publication_text_TypeRegularShift (VTL_publication_marked_text_markup_type_max + 1)
#define VTL_publication_text_TypeMax VTL_publication_text_type_regular_shift

typedef int VTL_publication_marked_text_type_flags;

#define VTL_publication_text_TypeStandardMd (1 << VTL_publication_markup_type_kStandardMD)
#define VTL_publication_text_TypeTelegramMd (1 << VTL_publication_markup_type_kTelegramMD)
#define VTL_publication_text_TypeHtml (1 << VTL_publication_markup_type_kHTML)
#define VTL_publication_text_TypeBb (1 << VTL_publication_markup_type_kBB)
#define VTL_publication_text_TypeRegular (1 << VTL_publication_text_type_regular_shift)

bool VTL_publication_MarkedTextTypeFlagCheckStandardMD(const VTL_publication_marked_text_TypeFlags flags);
bool VTL_publication_MarkedTextTypeFlagCheckTelegramMD(const VTL_publication_marked_text_TypeFlags flags);
bool VTL_publication_MarkedTextTypeFlagCheckHTML(const VTL_publication_marked_text_TypeFlags flags);
bool VTL_publication_MarkedTextTypeFlagCheckBB(const VTL_publication_marked_text_TypeFlags flags);
bool VTL_publication_MarkedTextTypeFlagCheckRegularText(const VTL_publication_marked_text_TypeFlags flags);

#ifdef __cplusplus
}
#endif

#endif
