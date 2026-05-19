#ifndef _VTL_PUBLICATION_TEXT_OP_HTML_DATA_H
#define _VTL_PUBLICATION_TEXT_OP_HTML_DATA_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/VTL_app_result.h>
#include <stddef.h>


// какой тег HTML нашёл сканер.
// Start/End — парные (открывающий <b>/<strong>/... и закрывающий </b>/...).
// Алиасы (<b>≡<strong>, <i>≡<em>, <s>≡<del>) на этом уровне уже схлопнуты
// в три семейства — конкретное имя нам после парсинга не нужно.
typedef enum _VTL_html_MarkerKind
{
    VTL_html_marker_kBoldStart = 0,
    VTL_html_marker_kBoldEnd,
    VTL_html_marker_kItalicStart,
    VTL_html_marker_kItalicEnd,
    VTL_html_marker_kStrikeStart,
    VTL_html_marker_kStrikeEnd
} VTL_html_MarkerKind;


// один найденный токен в исходном HTML.
// pos    — позиция '<' в src->text
// length — полная длина тега в байтах, включая '<' и '>' и возможные
//          атрибуты внутри. Нужна чтобы при конвертации правильно
//          "съесть" разметку и не утащить её в выход.
typedef struct _VTL_html_Marker
{
    VTL_html_MarkerKind kind;
    size_t pos;
    size_t length;
} VTL_html_Marker;


// динамический массив маркеров, растёт удвоением (амортизированный O(1) push)
// у каждого сканера в Parallel-режиме — свой такой буфер, поэтому без локов
typedef struct _VTL_html_MarkerList
{
    VTL_html_Marker* items;
    size_t length;
    size_t capacity;
} VTL_html_MarkerList;


// что сканер получает на вход: куда смотреть и куда складывать
typedef struct _VTL_html_ScanContext
{
    const char* text;
    size_t text_length;
    VTL_html_MarkerList* out;
} VTL_html_ScanContext;


VTL_AppResult VTL_html_MarkerListInit(VTL_html_MarkerList* list);
VTL_AppResult VTL_html_MarkerListReserve(VTL_html_MarkerList* list, size_t capacity);
VTL_AppResult VTL_html_MarkerListPush(VTL_html_MarkerList* list,
                                      VTL_html_Marker marker);
void VTL_html_MarkerListClear(VTL_html_MarkerList* list);
void VTL_html_MarkerListFree(VTL_html_MarkerList* list);


#ifdef __cplusplus
}
#endif

#endif
