#include <VTL/VTL_publication_markup_text_flags.h>

bool VTL_publication_marked_text_type_flag_CheckStandardMD(const VTL_publication_marked_text_type_flags flags) {
    return (flags & VTL_publication_text_type_standard_md) != 0;
}

bool VTL_publication_marked_text_type_flag_CheckTelegramMD(const VTL_publication_marked_text_type_flags flags) {
    return (flags & VTL_publication_text_type_telegram_md) != 0;
}

bool VTL_publication_marked_text_type_flag_CheckHTML(const VTL_publication_marked_text_type_flags flags) {
    return (flags & VTL_publication_text_type_html) != 0;
}

bool VTL_publication_marked_text_type_flag_CheckBB(const VTL_publication_marked_text_type_flags flags) {
    return (flags & VTL_publication_text_type_bb) != 0;
}

bool VTL_publication_marked_text_type_flag_CheckRegularText(const VTL_publication_marked_text_type_flags flags) {
    return (flags & VTL_publication_text_type_regular) != 0;
} 