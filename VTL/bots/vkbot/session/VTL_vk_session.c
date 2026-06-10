#include <VTL/bots/vkbot/session/VTL_vk_session.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static VTL_content_platform_flags vk_target_bit(const char* w)
{
    if (!strcmp(w, "vk"))                              return VTL_CONTENT_PLATFORM_VK;
    if (!strcmp(w, "w") || !strcmp(w, "wiki"))         return VTL_CONTENT_PLATFORM_W;
    if (!strcmp(w, "tg") || !strcmp(w, "telegram"))    return VTL_CONTENT_PLATFORM_TG;
    if (!strcmp(w, "r")  || !strcmp(w, "reddit"))      return VTL_CONTENT_PLATFORM_R;
    if (!strcmp(w, "vimeo"))                           return VTL_CONTENT_PLATFORM_VIMEO;
    if (!strcmp(w, "yt") || !strcmp(w, "youtube"))     return VTL_CONTENT_PLATFORM_YT;
    return 0;
}

int VTL_vk_ParseTarget(const char* word, VTL_content_platform_flags* out_bit)
{
    if (!word || !out_bit) return 0;
    char low[32]; size_t i = 0;
    for (; word[i] && i + 1 < sizeof(low); ++i) low[i] = (char)tolower((unsigned char)word[i]);
    low[i] = '\0';
    VTL_content_platform_flags b = vk_target_bit(low);
    if (!b) return 0;
    *out_bit = b;
    return 1;
}

int VTL_vk_ParseMarkup(const char* word, VTL_publication_marked_text_MarkupType* out)
{
    if (!word || !out) return 0;
    char low[32]; size_t i = 0;
    for (; word[i] && i + 1 < sizeof(low); ++i) low[i] = (char)tolower((unsigned char)word[i]);
    low[i] = '\0';

    if (!strcmp(low, "md")  || !strcmp(low, "standart")) *out = VTL_markup_type_kStandartMD;
    else if (!strcmp(low, "tg") || !strcmp(low, "telegram")) *out = VTL_markup_type_kTelegramMD;
    else if (!strcmp(low, "html"))                       *out = VTL_markup_type_kHTML;
    else if (!strcmp(low, "bb") || !strcmp(low, "bbcode")) *out = VTL_markup_type_kBB;
    else if (!strcmp(low, "adoc") || !strcmp(low, "asciidoc")) *out = VTL_markup_type_kAsciiDoc;
    else if (!strcmp(low, "wiki") || !strcmp(low, "mediawiki")) *out = VTL_markup_type_kMediaWiki;
    else return 0;
    return 1;
}

const char* VTL_vk_MarkupLabel(VTL_publication_marked_text_MarkupType m)
{
    switch (m) {
        case VTL_markup_type_kStandartMD: return "Markdown";
        case VTL_markup_type_kTelegramMD: return "Telegram MD";
        case VTL_markup_type_kHTML:       return "HTML";
        case VTL_markup_type_kBB:         return "BBCode";
        case VTL_markup_type_kAsciiDoc:   return "AsciiDoc";
        case VTL_markup_type_kMediaWiki:  return "MediaWiki";
        default:                          return "?";
    }
}

void VTL_vk_TargetsLabel(VTL_content_platform_flags flags, char* out, size_t cap)
{
    struct { const char* name; VTL_content_platform_flags bit; } tbl[] = {
        { "VK", VTL_CONTENT_PLATFORM_VK }, { "W", VTL_CONTENT_PLATFORM_W },
        { "TG", VTL_CONTENT_PLATFORM_TG }, { "Reddit", VTL_CONTENT_PLATFORM_R },
        { "Vimeo", VTL_CONTENT_PLATFORM_VIMEO }, { "YT", VTL_CONTENT_PLATFORM_YT },
    };
    size_t used = 0; int any = 0;
    if (cap) out[0] = '\0';
    for (size_t i = 0; i < sizeof(tbl)/sizeof(tbl[0]); ++i) {
        if (!(flags & tbl[i].bit)) continue;
        int n = snprintf(out + used, cap - used, "%s%s", any ? "+" : "", tbl[i].name);
        if (n < 0 || (size_t)n >= cap - used) break;
        used += (size_t)n; any = 1;
    }
    if (!any && cap) snprintf(out, cap, "%s", "ничего");
}

const char* VTL_vk_TargetsHelp(void)
{
    return "vk, w, tg, reddit, vimeo, yt";
}

const char* VTL_vk_MarkupsHelp(void)
{
    return "md, telegram, html, bb, asciidoc, mediawiki";
}

void VTL_vk_hub_Reset(VTL_vk_Hub* hub, const VTL_vk_Publishers* pub,
                      VTL_vk_EmitFn emit, void* emit_ud)
{
    if (!hub) return;
    memset(hub, 0, sizeof(*hub));
    hub->pub = pub;
    hub->emit = emit;
    hub->emit_ud = emit_ud;
}

void VTL_vk_session_ResetDefaults(VTL_vk_Dialog* d, long long peer_id)
{
    if (!d) return;
    d->used    = 1;
    d->peer_id = peer_id;
    d->targets = VTL_CONTENT_PLATFORM_VK;
    d->markup  = VTL_markup_type_kStandartMD;
    snprintf(d->source, sizeof(d->source), "%s", VTL_VK_DEFAULT_TEXT);
    d->token[0] = '\0';
}

VTL_vk_Dialog* VTL_vk_hub_Dialog(VTL_vk_Hub* hub, long long peer_id)
{
    if (!hub) return NULL;
    VTL_vk_Dialog* slot = NULL;
    for (size_t i = 0; i < VTL_VK_MAX_DIALOGS; ++i) {
        VTL_vk_Dialog* d = &hub->dialogs[i];
        if (d->used && d->peer_id == peer_id) return d;
        if (!d->used && !slot) slot = d;
    }
    if (!slot) return NULL;
    VTL_vk_session_ResetDefaults(slot, peer_id);
    return slot;
}

void VTL_vk_session_SetTargets(VTL_vk_Dialog* d, VTL_content_platform_flags flags)
{
    if (d) d->targets = flags;
}

void VTL_vk_session_SetMarkup(VTL_vk_Dialog* d, VTL_publication_marked_text_MarkupType markup)
{
    if (d) d->markup = markup;
}

void VTL_vk_session_SetSource(VTL_vk_Dialog* d, const char* path)
{
    if (d && path) snprintf(d->source, sizeof(d->source), "%s", path);
}

void VTL_vk_session_SetToken(VTL_vk_Dialog* d, const char* token)
{
    if (d && token) snprintf(d->token, sizeof(d->token), "%s", token);
}

void VTL_vk_session_ClearToken(VTL_vk_Dialog* d)
{
    if (d) d->token[0] = '\0';
}

int VTL_vk_session_HasToken(const VTL_vk_Dialog* d)
{
    return (d && d->token[0]) ? 1 : 0;
}
