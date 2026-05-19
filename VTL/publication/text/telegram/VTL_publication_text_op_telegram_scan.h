#ifndef _VTL_PUBLICATION_TEXT_OP_TELEGRAM_SCAN_H
#define _VTL_PUBLICATION_TEXT_OP_TELEGRAM_SCAN_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/publication/text/telegram/VTL_publication_text_op_telegram_data.h>
#include <VTL/publication/text/VTL_publication_text_data.h>
#include <VTL/VTL_app_result.h>


// сигнатура void*(void*) — совместима с pthread / WinAPI thread-обёрткой.
// Возвращает NULL при успехе, (void*)1 при OOM. В Sequential-режиме
// можно вызвать просто как обычную функцию.

// inline-разметка Telegram MarkdownV2:
void* VTL_telegram_ScanBold(void* ctx);    // *жирный*
void* VTL_telegram_ScanItalic(void* ctx);  // _курсив_
void* VTL_telegram_ScanStrike(void* ctx);  // ~зачёркнутый~


// запускает все сканеры и складывает результат в out (отсортированный по позиции)
// Sequential — все сканеры в одном потоке
// Parallel   — каждый сканер в своём потоке (pthread / WinAPI)
//              буферы у всех потоков свои, поэтому без локов
VTL_AppResult VTL_telegram_ParseDocumentSequential(const VTL_publication_Text* src,
                                                    VTL_telegram_MarkerList* out);

VTL_AppResult VTL_telegram_ParseDocumentParallel(const VTL_publication_Text* src,
                                                  VTL_telegram_MarkerList* out);


#ifdef __cplusplus
}
#endif

#endif
