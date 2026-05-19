#ifndef _VTL_PUBLICATION_TEXT_OP_HTML_CONVERT_H
#define _VTL_PUBLICATION_TEXT_OP_HTML_CONVERT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/publication/text/html/VTL_publication_text_op_html_data.h>
#include <VTL/publication/text/VTL_publication_text_data.h>
#include <VTL/VTL_app_result.h>
#include <stddef.h>


// HTML → внутренний формат.
// Проходит по отсортированным маркерам и нарезает текст на части с
// флагами BOLD/ITALIC/STRIKETHROUGH. Сами теги (<b>/<strong>/...)
// из выходного текста выкидываются. HTML-entities в обычном тексте
// при разборе тут НЕ декодируются — это пусть делает следующий слой,
// если потребуется (для round-trip и нужд большинства задач достаточно).
VTL_AppResult VTL_html_ConvertToMarkedText(const VTL_publication_Text* src,
                                            const VTL_html_MarkerList* markers,
                                            VTL_publication_MarkedText** out);

// Внутренний формат → HTML.
// Канонически использует короткие теги: <b>, <i>, <s>. Спецсимволы
// в тексте экранируются HTML-entities: '<' → &lt;, '>' → &gt;, '&' → &amp;.
VTL_AppResult VTL_html_SerializeFromMarkedText(const VTL_publication_MarkedText* src,
                                                VTL_publication_Text** out);


#ifdef __cplusplus
}
#endif

#endif
