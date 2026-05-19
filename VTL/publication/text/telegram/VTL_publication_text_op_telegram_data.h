#ifndef _VTL_PUBLICATION_TEXT_OP_TELEGRAM_DATA_H
#define _VTL_PUBLICATION_TEXT_OP_TELEGRAM_DATA_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/VTL_app_result.h>
#include <stddef.h>


// какой токен Telegram MarkdownV2 нашёл сканер
// Start/End — парные (открывающий и закрывающий звёздочка/подчерк/тильда)
typedef enum _VTL_telegram_MarkerKind
{
    VTL_telegram_marker_kBoldStart = 0,
    VTL_telegram_marker_kBoldEnd,
    VTL_telegram_marker_kItalicStart,
    VTL_telegram_marker_kItalicEnd,
    VTL_telegram_marker_kStrikeStart,
    VTL_telegram_marker_kStrikeEnd
} VTL_telegram_MarkerKind;


// один найденный токен в исходном тексте
// pos    — позиция первого байта токена в src->text
// length — длина токена в байтах (всегда 1 для *, _, ~; нужна, чтобы при
//          конвертации правильно "съесть" разметку и не утащить её в выход)
typedef struct _VTL_telegram_Marker
{
    VTL_telegram_MarkerKind kind;
    size_t pos;
    size_t length;
} VTL_telegram_Marker;


// динамический массив маркеров, растёт удвоением (амортизированный O(1) push)
// у каждого сканера в Parallel-режиме — свой такой буфер, поэтому без локов
typedef struct _VTL_telegram_MarkerList
{
    VTL_telegram_Marker* items;
    size_t length;
    size_t capacity;
} VTL_telegram_MarkerList;


// что сканер получает на вход: куда смотреть и куда складывать
typedef struct _VTL_telegram_ScanContext
{
    const char* text;
    size_t text_length;
    VTL_telegram_MarkerList* out;
} VTL_telegram_ScanContext;


VTL_AppResult VTL_telegram_MarkerListInit(VTL_telegram_MarkerList* list);
VTL_AppResult VTL_telegram_MarkerListReserve(VTL_telegram_MarkerList* list, size_t capacity);
VTL_AppResult VTL_telegram_MarkerListPush(VTL_telegram_MarkerList* list,
                                          VTL_telegram_Marker marker);
void VTL_telegram_MarkerListClear(VTL_telegram_MarkerList* list);
void VTL_telegram_MarkerListFree(VTL_telegram_MarkerList* list);


#ifdef __cplusplus
}
#endif

#endif
