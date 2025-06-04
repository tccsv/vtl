#include <VTL/publication/text/VTL_publication_text_data.h>


void VTL_publication_marked_text_modification_SetBold(VTL_publication_text_modification_Flags* p_flags)
{
    *p_flags |= VTL_TEXT_MODIFICATION_BOLD;
}

void VTL_publication_marked_text_modification_SetItalic(VTL_publication_text_modification_Flags* p_flags)
{
    *p_flags |= VTL_TEXT_MODIFICATION_ITALIC;
}

void VTL_publication_marked_text_modification_SetStrikethrough(VTL_publication_text_modification_Flags* p_flags)
{
    *p_flags |= VTL_TEXT_MODIFICATION_STRIKETHROUGH;
}

