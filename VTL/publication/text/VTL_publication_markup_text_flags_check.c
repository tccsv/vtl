#include <VTL/VTL_publication_markup_text_flags.h>

/* В оригинале здесь была precedence-ошибка вида
 *     flags & X == X
 * что парсится как flags & (X == X) и почти всегда даёт мусор.
 * Исправлено на (flags & X) == X. */

bool VTL_publication_marked_text_type_flag_CheckStandartMD(const VTL_publication_marked_text_type_Flags flags)
{
    return (flags & VTL_PUBLICATION_TEXT_TYPE_STANDART_MD) == VTL_PUBLICATION_TEXT_TYPE_STANDART_MD;
}

bool VTL_publication_marked_text_type_flag_CheckTelegramMD(const VTL_publication_marked_text_type_Flags flags)
{
    return (flags & VTL_PUBLICATION_TEXT_TYPE_TELEGRAM_MD) == VTL_PUBLICATION_TEXT_TYPE_TELEGRAM_MD;
}

bool VTL_publication_marked_text_type_flag_CheckHTML(const VTL_publication_marked_text_type_Flags flags)
{
    return (flags & VTL_PUBLICATION_TEXT_TYPE_HTML) == VTL_PUBLICATION_TEXT_TYPE_HTML;
}

bool VTL_publication_marked_text_type_flag_CheckBB(const VTL_publication_marked_text_type_Flags flags)
{
    return (flags & VTL_PUBLICATION_TEXT_TYPE_BB) == VTL_PUBLICATION_TEXT_TYPE_BB;
}

bool VTL_publication_marked_text_type_flag_CheckMediaWiki(const VTL_publication_marked_text_type_Flags flags)
{
    return (flags & VTL_PUBLICATION_TEXT_TYPE_MEDIAWIKI) == VTL_PUBLICATION_TEXT_TYPE_MEDIAWIKI;
}

bool VTL_publication_marked_text_type_flag_CheckRegularText(const VTL_publication_marked_text_type_Flags flags)
{
    return (flags & VTL_PUBLICATION_TEXT_TYPE_REGULAR) == VTL_PUBLICATION_TEXT_TYPE_REGULAR;
}
