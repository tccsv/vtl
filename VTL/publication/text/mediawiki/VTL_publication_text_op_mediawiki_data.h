#ifndef _VTL_PUBLICATION_TEXT_OP_MEDIAWIKI_DATA_H
#define _VTL_PUBLICATION_TEXT_OP_MEDIAWIKI_DATA_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/VTL_app_result.h>
#include <stddef.h>


typedef enum _VTL_mediawiki_MarkerKind
{
    VTL_mediawiki_marker_kBoldStart = 0,
    VTL_mediawiki_marker_kBoldEnd,
    VTL_mediawiki_marker_kItalicStart,
    VTL_mediawiki_marker_kItalicEnd,
    VTL_mediawiki_marker_kStrikeStart,
    VTL_mediawiki_marker_kStrikeEnd,

    VTL_mediawiki_marker_kHeading,
    VTL_mediawiki_marker_kListItem,
    VTL_mediawiki_marker_kInternalLink,
    VTL_mediawiki_marker_kExternalLink,
    VTL_mediawiki_marker_kTemplate,
    VTL_mediawiki_marker_kNowikiStart,
    VTL_mediawiki_marker_kNowikiEnd,
    VTL_mediawiki_marker_kComment,
    VTL_mediawiki_marker_kHorizontalRule,
    VTL_mediawiki_marker_kRef,
    VTL_mediawiki_marker_kGenericTag
} VTL_mediawiki_MarkerKind;


// For kGenericTag: level encodes opening-tag length, aux encodes closing-tag length.
// For kHeading:    level holds the '=' count on each side.
// For kListItem:   level holds the nesting depth.
typedef struct _VTL_mediawiki_Marker
{
    VTL_mediawiki_MarkerKind kind;
    size_t pos;
    size_t length;
    int level;
    int aux;
} VTL_mediawiki_Marker;


typedef struct _VTL_mediawiki_MarkerList
{
    VTL_mediawiki_Marker* items;
    size_t length;
    size_t capacity;
} VTL_mediawiki_MarkerList;


typedef struct _VTL_mediawiki_ScanContext
{
    const char* text;
    size_t text_length;
    VTL_mediawiki_MarkerList* out;
} VTL_mediawiki_ScanContext;


VTL_AppResult VTL_mediawiki_MarkerListInit(VTL_mediawiki_MarkerList* list);
VTL_AppResult VTL_mediawiki_MarkerListReserve(VTL_mediawiki_MarkerList* list, size_t capacity);
VTL_AppResult VTL_mediawiki_MarkerListPush(VTL_mediawiki_MarkerList* list,
                                            VTL_mediawiki_Marker marker);
void VTL_mediawiki_MarkerListClear(VTL_mediawiki_MarkerList* list);
void VTL_mediawiki_MarkerListFree(VTL_mediawiki_MarkerList* list);


#ifdef __cplusplus
}
#endif

#endif
