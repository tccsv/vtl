#ifndef _VTL_PUBLICATION_TEXT_OP_ASCIIDOC_SCAN_H
#define _VTL_PUBLICATION_TEXT_OP_ASCIIDOC_SCAN_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/publication/text/asciidoc/VTL_publication_text_op_asciidoc_data.h>
#include <VTL/publication/text/VTL_publication_text_data.h>
#include <VTL/VTL_app_result.h>


// сигнатура void*(void*) — совместима с pthread / WinAPI thread-обёрткой
// возвращает NULL при успехе, (void*)1 при OOM

// inline-разметка
void* VTL_asciidoc_ScanBold(void* ctx);          // *жирный*
void* VTL_asciidoc_ScanItalic(void* ctx);        // _курсив_
void* VTL_asciidoc_ScanMono(void* ctx);          // `моноширинный`
void* VTL_asciidoc_ScanSubscript(void* ctx);     // ~нижний~
void* VTL_asciidoc_ScanSuperscript(void* ctx);   // ^верхний^
void* VTL_asciidoc_ScanStrikethrough(void* ctx); // [line-through]#зачёркнутый#
void* VTL_asciidoc_ScanLink(void* ctx);          // link:url[текст]
void* VTL_asciidoc_ScanCrossReference(void* ctx);// <<id>> или <<id,текст>>

// блочная (по строкам)
void* VTL_asciidoc_ScanHeading(void* ctx);       // = Заголовок ... ====== уровни 1..6
void* VTL_asciidoc_ScanList(void* ctx);          // * пункт или . нумерованный
void* VTL_asciidoc_ScanCodeBlock(void* ctx);     // ---- ... ----
void* VTL_asciidoc_ScanQuotedBlock(void* ctx);   // ____ ... ____
void* VTL_asciidoc_ScanComment(void* ctx);       // // до конца строки

// уровень документа
void* VTL_asciidoc_ScanAttribute(void* ctx);     // :ключ: значение
void* VTL_asciidoc_ScanAdmonition(void* ctx);    // NOTE: / TIP: / WARNING: / ...


// запускает все 15 сканеров и сливает результат в out (уже отсортированный)
// Sequential — один поток, Parallel — pthread на каждый сканер
VTL_AppResult VTL_asciidoc_ParseDocumentSequential(const VTL_publication_Text* src,
                                                    VTL_asciidoc_MarkerList* out);

VTL_AppResult VTL_asciidoc_ParseDocumentParallel(const VTL_publication_Text* src,
                                                  VTL_asciidoc_MarkerList* out);


#ifdef __cplusplus
}
#endif

#endif
