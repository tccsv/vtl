#ifndef _VTL_PUBLICATION_TEXT_OP_HTML_SCAN_H
#define _VTL_PUBLICATION_TEXT_OP_HTML_SCAN_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/publication/text/html/VTL_publication_text_op_html_data.h>
#include <VTL/publication/text/VTL_publication_text_data.h>
#include <VTL/VTL_app_result.h>


// сигнатура void*(void*) — совместима с pthread / WinAPI thread-обёрткой.
// Возвращает NULL при успехе, (void*)1 при OOM. В Sequential-режиме
// можно вызвать просто как обычную функцию.

// inline-разметка HTML, по семействам тегов:
void* VTL_html_ScanBold(void* ctx);     // <b>...</b>      и <strong>...</strong>
void* VTL_html_ScanItalic(void* ctx);   // <i>...</i>      и <em>...</em>
void* VTL_html_ScanStrike(void* ctx);   // <s>...</s>      и <del>...</del>


// запускает все сканеры и складывает результат в out (отсортированный по позиции)
// Sequential — все сканеры в одном потоке
// Parallel   — каждый сканер в своём потоке (pthread / WinAPI)
//              буферы у всех потоков свои, поэтому без локов
VTL_AppResult VTL_html_ParseDocumentSequential(const VTL_publication_Text* src,
                                                VTL_html_MarkerList* out);

VTL_AppResult VTL_html_ParseDocumentParallel(const VTL_publication_Text* src,
                                              VTL_html_MarkerList* out);


#ifdef __cplusplus
}
#endif

#endif
