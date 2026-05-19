#ifndef _VTL_PUBLICATION_TEXT_OP_HTML_H
#define _VTL_PUBLICATION_TEXT_OP_HTML_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/publication/text/html/VTL_publication_text_op_html_data.h>
#include <VTL/publication/text/html/VTL_publication_text_op_html_scan.h>
#include <VTL/publication/text/html/VTL_publication_text_op_html_convert.h>
#include <VTL/publication/text/VTL_publication_text_data.h>
#include <VTL/VTL_app_result.h>
#include <stddef.h>


// HTML → внутренний MarkedText.
// Sequential — все сканеры в одном потоке (для отладки и маленьких текстов).
// Parallel   — каждый сканер в своём потоке (pthread / WinAPI),
//              у каждого свой буфер маркеров, поэтому без локов.
//
// Поддерживается минимальный набор inline-разметки:
//   <b>...</b>      и <strong>...</strong>   → BOLD
//   <i>...</i>      и <em>...</em>           → ITALIC
//   <s>...</s>      и <del>...</del>         → STRIKETHROUGH
// Имена тегов case-insensitive, атрибуты в открывающем теге пропускаются.
VTL_AppResult VTL_html_ParseTextSequential(const VTL_publication_Text* src,
                                            VTL_publication_MarkedText** out);

VTL_AppResult VTL_html_ParseTextParallel(const VTL_publication_Text* src,
                                          VTL_publication_MarkedText** out);


// MarkedText → HTML-текст. Один поток, потому что сериализация — это
// линейная сборка, она и так O(N) и упирается в memcpy.
// Канонические теги: <b>, <i>, <s>. Спецсимволы '<', '>', '&' в тексте
// экранируются HTML-entities (&lt; &gt; &amp;).
VTL_AppResult VTL_html_SerializeText(const VTL_publication_MarkedText* src,
                                      VTL_publication_Text** out);


// Замер времени двух режимов, секунды (wall-clock, не CPU-time).
// iterations — сколько прогонов усреднить.
double VTL_html_BenchSequential(const VTL_publication_Text* src, size_t iterations);
double VTL_html_BenchParallel(const VTL_publication_Text* src, size_t iterations);


#ifdef __cplusplus
}
#endif

#endif
