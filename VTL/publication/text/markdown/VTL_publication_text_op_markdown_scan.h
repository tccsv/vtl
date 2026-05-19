#ifndef _VTL_PUBLICATION_TEXT_OP_MARKDOWN_SCAN_H
#define _VTL_PUBLICATION_TEXT_OP_MARKDOWN_SCAN_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/publication/text/markdown/VTL_publication_text_op_markdown_data.h>
#include <VTL/publication/text/VTL_publication_text_data.h>
#include <VTL/VTL_app_result.h>


// сигнатура void*(void*) — совместима с pthread / WinAPI thread-обёрткой.
// Возвращает NULL при успехе, (void*)1 при OOM. В Sequential-режиме
// вызывается просто как обычная функция.

// inline-разметка Standart Markdown / CommonMark / GFM:
void* VTL_markdown_ScanBold(void* ctx);    // **жирный** или __жирный__
void* VTL_markdown_ScanItalic(void* ctx);  // *курсив* или _курсив_
void* VTL_markdown_ScanStrike(void* ctx);  // ~~зачёркнутый~~ (GFM)


// запускает все сканеры и складывает результат в out (отсортированный по позиции)
// Sequential — все сканеры в одном потоке
// Parallel   — каждый сканер в своём потоке (pthread / WinAPI),
//              буферы у всех потоков свои, поэтому без локов
VTL_AppResult VTL_markdown_ParseDocumentSequential(const VTL_publication_Text* src,
                                                    VTL_markdown_MarkerList* out);

VTL_AppResult VTL_markdown_ParseDocumentParallel(const VTL_publication_Text* src,
                                                  VTL_markdown_MarkerList* out);


#ifdef __cplusplus
}
#endif

#endif
