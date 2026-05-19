#ifndef _VTL_PUBLICATION_TEXT_OP_MARKDOWN_DATA_H
#define _VTL_PUBLICATION_TEXT_OP_MARKDOWN_DATA_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/VTL_app_result.h>
#include <stddef.h>


// какой токен Standart Markdown нашёл сканер
// Start/End — парные (открывающий и закрывающий)
typedef enum _VTL_markdown_MarkerKind
{
    VTL_markdown_marker_kBoldStart = 0,
    VTL_markdown_marker_kBoldEnd,
    VTL_markdown_marker_kItalicStart,
    VTL_markdown_marker_kItalicEnd,
    VTL_markdown_marker_kStrikeStart,
    VTL_markdown_marker_kStrikeEnd
} VTL_markdown_MarkerKind;


// один найденный токен в исходном тексте
// pos    — позиция первого байта токена в src->text
// length — сколько байт занимает токен (1 для *, _; 2 для **, __, ~~).
//          критично для ConvertToMarkedText, чтобы "съесть" правильное число байт
typedef struct _VTL_markdown_Marker
{
    VTL_markdown_MarkerKind kind;
    size_t pos;
    size_t length;
} VTL_markdown_Marker;


// динамический массив маркеров, растёт удвоением (амортизированный O(1) push)
// у каждого сканера в Parallel-режиме — свой такой буфер, поэтому без локов
typedef struct _VTL_markdown_MarkerList
{
    VTL_markdown_Marker* items;
    size_t length;
    size_t capacity;
} VTL_markdown_MarkerList;


// что сканер получает на вход: куда смотреть и куда складывать
typedef struct _VTL_markdown_ScanContext
{
    const char* text;
    size_t text_length;
    VTL_markdown_MarkerList* out;
} VTL_markdown_ScanContext;


VTL_AppResult VTL_markdown_MarkerListInit(VTL_markdown_MarkerList* list);
VTL_AppResult VTL_markdown_MarkerListReserve(VTL_markdown_MarkerList* list, size_t capacity);
VTL_AppResult VTL_markdown_MarkerListPush(VTL_markdown_MarkerList* list,
                                          VTL_markdown_Marker marker);
void VTL_markdown_MarkerListClear(VTL_markdown_MarkerList* list);
void VTL_markdown_MarkerListFree(VTL_markdown_MarkerList* list);


#ifdef __cplusplus
}
#endif

#endif
