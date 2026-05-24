#ifndef VTL_PUBLICATION_TEXT_OP_DISCORD_H
#define VTL_PUBLICATION_TEXT_OP_DISCORD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <VTL/publication/text/VTL_publication_text_data.h>
#include <VTL/VTL_app_result.h>
#include <stddef.h>

/*
 * Discord Markdown ↔ внутренний формат.
 *
 *   **text**   → BOLD
 *   *text*     → ITALIC
 *   _text_     → ITALIC
 *   ***text*** → BOLD | ITALIC
 *   ~~text~~   → STRIKETHROUGH
 */

typedef enum _VTL_discord_MarkerKind {
    VTL_discord_marker_kBoldStart = 0,
    VTL_discord_marker_kBoldEnd,
    VTL_discord_marker_kItalicStart,
    VTL_discord_marker_kItalicEnd,
    VTL_discord_marker_kStrikeStart,
    VTL_discord_marker_kStrikeEnd,
    VTL_discord_marker_kBoldItalicStart,
    VTL_discord_marker_kBoldItalicEnd
} VTL_discord_MarkerKind;

typedef struct {
    VTL_discord_MarkerKind kind;
    size_t pos;
    size_t length;
} VTL_discord_Marker;

typedef struct {
    VTL_discord_Marker *items;
    size_t length;
    size_t capacity;
} VTL_discord_MarkerList;

typedef struct {
    const char *text;
    size_t text_length;
    VTL_discord_MarkerList *out;
} VTL_discord_ScanContext;

#define VTL_DISCORD_SCANNER_COUNT 5

VTL_AppResult VTL_discord_ParseTextSequential(const VTL_publication_Text *src, VTL_publication_MarkedText **out);
VTL_AppResult VTL_discord_ParseTextParallel(const VTL_publication_Text *src, VTL_publication_MarkedText **out);
VTL_AppResult VTL_discord_SerializeToText(const VTL_publication_MarkedText *src, VTL_publication_Text **out);

#ifdef __cplusplus
}
#endif

#endif /* VTL_PUBLICATION_TEXT_OP_DISCORD_H */
