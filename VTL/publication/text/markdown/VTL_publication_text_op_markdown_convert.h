#ifndef _VTL_PUBLICATION_TEXT_OP_MARKDOWN_CONVERT_H
#define _VTL_PUBLICATION_TEXT_OP_MARKDOWN_CONVERT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/publication/text/markdown/VTL_publication_text_op_markdown_data.h>
#include <VTL/publication/text/VTL_publication_text_data.h>
#include <VTL/VTL_app_result.h>
#include <stddef.h>


// Standart Markdown → внутренний формат.
// Проходит по отсортированным маркерам и нарезает текст на части с
// флагами BOLD/ITALIC/STRIKETHROUGH. Сами токены разметки (*, **, _, __, ~~)
// из выходного текста выкидываются.
VTL_AppResult VTL_markdown_ConvertToMarkedText(const VTL_publication_Text* src,
                                                const VTL_markdown_MarkerList* markers,
                                                VTL_publication_MarkedText** out);

// Внутренний формат → Standart Markdown.
// Выдаёт канонический синтаксис: ** для bold, * для italic, ~~ для strike.
// Спецсимволы (* _ ~ \) внутри текста экранируются обратным слешем.
VTL_AppResult VTL_markdown_SerializeFromMarkedText(const VTL_publication_MarkedText* src,
                                                    VTL_publication_Text** out);


#ifdef __cplusplus
}
#endif

#endif
