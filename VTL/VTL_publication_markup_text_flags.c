#include <VTL/VTL_publication_markup_TextFlags.h>

bool VTL_publication_MarkedTextTypeFlagCheckStandardMD(const VTL_publication_marked_text_TypeFlags flags) {
    return (flags & VTL_publication_text_type_standard_md) != 0;
}

bool VTL_publication_MarkedTextTypeFlagCheckTelegramMD(const VTL_publication_marked_text_TypeFlags flags) {
    return (flags & VTL_publication_text_type_telegram_md) != 0;
}

bool VTL_publication_MarkedTextTypeFlagCheckHTML(const VTL_publication_marked_text_TypeFlags flags) {
    return (flags & VTL_publication_text_type_html) != 0;
}

bool VTL_publication_MarkedTextTypeFlagCheckBB(const VTL_publication_marked_text_TypeFlags flags) {
    return (flags & VTL_publication_text_type_bb) != 0;
}

bool VTL_publication_MarkedTextTypeFlagCheckRegularText(const VTL_publication_marked_text_TypeFlags flags) {
    return (flags & VTL_publication_text_type_regular) != 0;
} 
