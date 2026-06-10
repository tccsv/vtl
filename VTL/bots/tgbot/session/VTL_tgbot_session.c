#include <VTL/bots/tgbot/session/VTL_tgbot_session.h>
#include <VTL/bots/tgbot/VTL_tgbot_data.h>

#include <stdio.h>
#include <string.h>

void VTL_tgbot_session_TableInit(VTL_tgbot_sessionTable* table)
{
    if (table) memset(table, 0, sizeof(*table));
}

static void VTL_tgbot_session_ResetDefaults(VTL_tgbot_session* s, const char* chat_id)
{
    snprintf(s->chat_id, sizeof(s->chat_id), "%s", chat_id);
    s->platforms = VTL_CONTENT_PLATFORM_W | VTL_CONTENT_PLATFORM_TG;
    s->markup    = VTL_markup_type_kTelegramMD;
    snprintf(s->text_file, sizeof(s->text_file), "%s", VTL_tgbot_DEFAULT_TEXT);
    s->token[0] = '\0';
    s->in_use = 1;
}

VTL_tgbot_session* VTL_tgbot_session_GetOrCreate(VTL_tgbot_sessionTable* table,
                                             const char* chat_id)
{
    if (!table || !chat_id || !*chat_id) return NULL;

    VTL_tgbot_session* free_slot = NULL;
    for (size_t i = 0; i < VTL_tgbot_session_MAX_CHATS; ++i) {
        VTL_tgbot_session* s = &table->items[i];
        if (s->in_use) {
            if (strcmp(s->chat_id, chat_id) == 0) return s;
        } else if (!free_slot) {
            free_slot = s;
        }
    }
    if (!free_slot) return NULL;

    VTL_tgbot_session_ResetDefaults(free_slot, chat_id);
    return free_slot;
}

typedef struct
{
    const char*                 name;
    VTL_content_platform_flags  bit;
} VTL_tgbot_session_PlatformEntry;

static const VTL_tgbot_session_PlatformEntry VTL_tgbot_session_kPlatforms[] = {
    { "tg",       VTL_CONTENT_PLATFORM_TG    },
    { "telegram", VTL_CONTENT_PLATFORM_TG    },
    { "w",        VTL_CONTENT_PLATFORM_W     },
    { "wiki",     VTL_CONTENT_PLATFORM_W     },
    { "web",      VTL_CONTENT_PLATFORM_W     },
    { "r",        VTL_CONTENT_PLATFORM_R     },
    { "reddit",   VTL_CONTENT_PLATFORM_R     },
    { "vimeo",    VTL_CONTENT_PLATFORM_VIMEO },
    { "yt",       VTL_CONTENT_PLATFORM_YT    },
    { "youtube",  VTL_CONTENT_PLATFORM_YT    },
    { "vk",       VTL_CONTENT_PLATFORM_VK    },
};

typedef struct
{
    const char*                 label;
    VTL_content_platform_flags  bit;
} VTL_tgbot_session_PlatformLabel;

static const VTL_tgbot_session_PlatformLabel VTL_tgbot_session_kLabels[] = {
    { "W",     VTL_CONTENT_PLATFORM_W     },
    { "TG",    VTL_CONTENT_PLATFORM_TG    },
    { "Reddit",VTL_CONTENT_PLATFORM_R     },
    { "Vimeo", VTL_CONTENT_PLATFORM_VIMEO },
    { "YT",    VTL_CONTENT_PLATFORM_YT    },
    { "VK",    VTL_CONTENT_PLATFORM_VK    },
};

typedef struct
{
    const char*                             name;
    VTL_publication_marked_text_MarkupType  markup;
} VTL_tgbot_session_FormatEntry;

static const VTL_tgbot_session_FormatEntry VTL_tgbot_session_kFormats[] = {
    { "md",        VTL_markup_type_kStandartMD },
    { "standart",  VTL_markup_type_kStandartMD },
    { "standard",  VTL_markup_type_kStandartMD },
    { "tg",        VTL_markup_type_kTelegramMD },
    { "telegram",  VTL_markup_type_kTelegramMD },
    { "tgmd",      VTL_markup_type_kTelegramMD },
    { "html",      VTL_markup_type_kHTML       },
    { "bb",        VTL_markup_type_kBB         },
    { "bbcode",    VTL_markup_type_kBB         },
    { "adoc",      VTL_markup_type_kAsciiDoc   },
    { "asciidoc",  VTL_markup_type_kAsciiDoc   },
    { "wiki",      VTL_markup_type_kMediaWiki  },
    { "mediawiki", VTL_markup_type_kMediaWiki  },
};

static int VTL_tgbot_session_EqualsCI(const char* a, const char* b)
{
    while (*a && *b) {
        char ca = *a, cb = *b;
        if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
        if (ca != cb) return 0;
        ++a; ++b;
    }
    return *a == *b;
}

int VTL_tgbot_session_ParsePlatform(const char* token,
                                  VTL_content_platform_flags* out_bit)
{
    if (!token || !out_bit) return 0;
    const size_t n = sizeof(VTL_tgbot_session_kPlatforms) /
                     sizeof(VTL_tgbot_session_kPlatforms[0]);
    for (size_t i = 0; i < n; ++i) {
        if (VTL_tgbot_session_EqualsCI(token, VTL_tgbot_session_kPlatforms[i].name)) {
            *out_bit = VTL_tgbot_session_kPlatforms[i].bit;
            return 1;
        }
    }
    return 0;
}

int VTL_tgbot_session_ParseFormat(const char* token,
                                VTL_publication_marked_text_MarkupType* out_markup)
{
    if (!token || !out_markup) return 0;
    const size_t n = sizeof(VTL_tgbot_session_kFormats) /
                     sizeof(VTL_tgbot_session_kFormats[0]);
    for (size_t i = 0; i < n; ++i) {
        if (VTL_tgbot_session_EqualsCI(token, VTL_tgbot_session_kFormats[i].name)) {
            *out_markup = VTL_tgbot_session_kFormats[i].markup;
            return 1;
        }
    }
    return 0;
}

const char* VTL_tgbot_session_FormatName(VTL_publication_marked_text_MarkupType markup)
{
    switch (markup) {
        case VTL_markup_type_kStandartMD: return "Standard MD";
        case VTL_markup_type_kTelegramMD: return "Telegram MD";
        case VTL_markup_type_kHTML:       return "HTML";
        case VTL_markup_type_kBB:         return "BBCode";
        case VTL_markup_type_kAsciiDoc:   return "AsciiDoc";
        case VTL_markup_type_kMediaWiki:  return "MediaWiki";
        default:                          return "?";
    }
}

void VTL_tgbot_session_DescribePlatforms(VTL_content_platform_flags flags,
                                       char* out, size_t cap)
{
    if (!out || cap == 0) return;
    out[0] = '\0';

    size_t used = 0;
    int    first = 1;
    const size_t n = sizeof(VTL_tgbot_session_kLabels) /
                     sizeof(VTL_tgbot_session_kLabels[0]);
    for (size_t i = 0; i < n; ++i) {
        if ((flags & VTL_tgbot_session_kLabels[i].bit) == 0) continue;
        int written = snprintf(out + used, cap - used, "%s%s",
                               first ? "" : ", ",
                               VTL_tgbot_session_kLabels[i].label);
        if (written < 0 || (size_t)written >= cap - used) break;
        used += (size_t)written;
        first = 0;
    }
    if (first) snprintf(out, cap, "%s", "—");
}

const char* VTL_tgbot_session_PlatformsHelp(void)
{
    return "tg, w, reddit, vimeo, yt, vk";
}

const char* VTL_tgbot_session_FormatsHelp(void)
{
    return "md, telegram, html, bb, asciidoc, mediawiki";
}
