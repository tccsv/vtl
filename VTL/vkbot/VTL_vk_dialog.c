#include <VTL/vkbot/VTL_vk_dialog.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VTL_VK_REPLY_MAX 4000

/* ---- словари площадок и форматов ---- */

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

/* ---- хаб ---- */

void VTL_vk_hub_Reset(VTL_vk_Hub* hub, const VTL_vk_Publishers* pub,
                      VTL_vk_EmitFn emit, void* emit_ud)
{
    if (!hub) return;
    memset(hub, 0, sizeof(*hub));
    hub->pub = pub;
    hub->emit = emit;
    hub->emit_ud = emit_ud;
}

VTL_vk_Dialog* VTL_vk_hub_Dialog(VTL_vk_Hub* hub, long long peer_id)
{
    VTL_vk_Dialog* slot = NULL;
    for (size_t i = 0; i < VTL_VK_MAX_DIALOGS; ++i) {
        VTL_vk_Dialog* d = &hub->dialogs[i];
        if (d->used && d->peer_id == peer_id) return d;
        if (!d->used && !slot) slot = d;
    }
    if (!slot) return NULL;
    slot->used    = 1;
    slot->peer_id = peer_id;
    slot->targets = VTL_CONTENT_PLATFORM_VK;       /* по умолчанию — ВК */
    slot->markup  = VTL_markup_type_kStandartMD;
    snprintf(slot->source, sizeof(slot->source), "%s", "text.md");
    return slot;
}

static void vk_say(VTL_vk_Hub* hub, long long peer, const char* text)
{
    if (hub->emit) hub->emit(hub->emit_ud, peer, text);
}

static int vk_file_ok(const char* path)
{
    FILE* f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

static const char VK_HELP[] =
    "Бот публикации VTL (сообщество ВК).\n"
    "Выбираешь площадки и разметку — командой /post запускаешь публикацию.\n\n"
    "/targets <список> — площадки: vk, w, tg, reddit, vimeo, yt\n"
    "/markup <тип> — разметка: md, telegram, html, bb, asciidoc, mediawiki\n"
    "/source <файл> — файл с текстом\n"
    "/show — что выбрано сейчас\n"
    "/post — опубликовать текст\n"
    "/post_audio <файл> — опубликовать аудио с подписью\n"
    "/me — мой peer_id\n"
    "/ping — проверка";

/* первое слово (в нижнем регистре, без ведущего '/') в cmd; возврат — хвост */
static const char* vk_split(const char* s, char* cmd, size_t cap)
{
    while (*s == ' ' || *s == '\t') ++s;
    if (*s == '/') ++s;
    size_t n = 0;
    while (s[n] && s[n] != ' ' && s[n] != '\t' && n + 1 < cap) {
        cmd[n] = (char)tolower((unsigned char)s[n]); ++n;
    }
    cmd[n] = '\0';
    s += n;
    while (*s == ' ' || *s == '\t') ++s;
    return s;
}

/* первый токен из args в tok */
static void vk_first(const char* args, char* tok, size_t cap)
{
    size_t n = 0;
    while (args[n] && args[n] != ' ' && args[n] != '\t' && n + 1 < cap) { tok[n] = args[n]; ++n; }
    tok[n] = '\0';
}

void VTL_vk_hub_Handle(VTL_vk_Hub* hub, long long peer_id, const char* text)
{
    if (!hub || !text) return;
    VTL_vk_Dialog* d = VTL_vk_hub_Dialog(hub, peer_id);
    if (!d) { vk_say(hub, peer_id, "Слишком много диалогов, позже."); return; }

    char cmd[32];
    const char* args = vk_split(text, cmd, sizeof(cmd));
    char reply[VTL_VK_REPLY_MAX];
    char tlabel[160];

    if (!cmd[0] || !strcmp(cmd, "help") || !strcmp(cmd, "start")) {
        vk_say(hub, peer_id, VK_HELP);

    } else if (!strcmp(cmd, "targets")) {
        if (!*args) {
            VTL_vk_TargetsLabel(d->targets, tlabel, sizeof(tlabel));
            snprintf(reply, sizeof(reply), "Площадки: %s. Доступно: vk, w, tg, reddit, vimeo, yt", tlabel);
            vk_say(hub, peer_id, reply); return;
        }
        VTL_content_platform_flags acc = 0; char tok[32]; const char* p = args;
        while (*p) {
            vk_first(p, tok, sizeof(tok));
            VTL_content_platform_flags b;
            if (*tok && VTL_vk_ParseTarget(tok, &b)) acc |= b;
            while (*p && *p != ' ') ++p; while (*p == ' ') ++p;
        }
        if (!acc) { vk_say(hub, peer_id, "Не понял ни одной площадки."); return; }
        d->targets = acc;
        VTL_vk_TargetsLabel(d->targets, tlabel, sizeof(tlabel));
        snprintf(reply, sizeof(reply), "Ок, площадки: %s", tlabel);
        vk_say(hub, peer_id, reply);

    } else if (!strcmp(cmd, "markup")) {
        char tok[32]; vk_first(args, tok, sizeof(tok));
        if (!*tok) {
            snprintf(reply, sizeof(reply), "Разметка: %s. Доступно: md, telegram, html, bb, asciidoc, mediawiki",
                     VTL_vk_MarkupLabel(d->markup));
            vk_say(hub, peer_id, reply); return;
        }
        VTL_publication_marked_text_MarkupType m;
        if (!VTL_vk_ParseMarkup(tok, &m)) { vk_say(hub, peer_id, "Такой разметки нет."); return; }
        d->markup = m;
        snprintf(reply, sizeof(reply), "Разметка: %s", VTL_vk_MarkupLabel(m));
        vk_say(hub, peer_id, reply);

    } else if (!strcmp(cmd, "source")) {
        char tok[VTL_VK_PATH_MAX]; vk_first(args, tok, sizeof(tok));
        if (!*tok) { snprintf(reply, sizeof(reply), "Файл: %s", d->source); vk_say(hub, peer_id, reply); return; }
        snprintf(d->source, sizeof(d->source), "%s", tok);
        snprintf(reply, sizeof(reply), "Файл: %s", d->source);
        vk_say(hub, peer_id, reply);

    } else if (!strcmp(cmd, "show")) {
        VTL_vk_TargetsLabel(d->targets, tlabel, sizeof(tlabel));
        snprintf(reply, sizeof(reply), "Файл: %s\nПлощадки: %s\nРазметка: %s",
                 d->source, tlabel, VTL_vk_MarkupLabel(d->markup));
        vk_say(hub, peer_id, reply);

    } else if (!strcmp(cmd, "post")) {
        if (!hub->pub || !hub->pub->text) { vk_say(hub, peer_id, "Публикация текста недоступна."); return; }
        if (!vk_file_ok(d->source)) {
            snprintf(reply, sizeof(reply), "Нет файла: %s (задай /source)", d->source);
            vk_say(hub, peer_id, reply); return;
        }
        VTL_vk_TargetsLabel(d->targets, tlabel, sizeof(tlabel));
        printf("[vk] post text: %s -> %s (%s)\n", d->source, tlabel, VTL_vk_MarkupLabel(d->markup));
        fflush(stdout);
        VTL_AppResult rc = hub->pub->text(d->source, d->targets, d->markup);
        snprintf(reply, sizeof(reply), "Текст: %s (код %d) [%s, %s]",
                 rc == VTL_res_kOk ? "опубликовано" : "ошибка", (int)rc,
                 tlabel, VTL_vk_MarkupLabel(d->markup));
        vk_say(hub, peer_id, reply);

    } else if (!strcmp(cmd, "post_audio")) {
        char audio[VTL_VK_PATH_MAX]; vk_first(args, audio, sizeof(audio));
        if (!*audio) { vk_say(hub, peer_id, "Укажи аудиофайл: /post_audio file.mp3"); return; }
        if (!hub->pub || !hub->pub->audio) { vk_say(hub, peer_id, "Публикация аудио недоступна."); return; }
        if (!vk_file_ok(audio))     { snprintf(reply, sizeof(reply), "Нет аудио: %s", audio); vk_say(hub, peer_id, reply); return; }
        if (!vk_file_ok(d->source)) { snprintf(reply, sizeof(reply), "Нет файла текста: %s", d->source); vk_say(hub, peer_id, reply); return; }
        VTL_vk_TargetsLabel(d->targets, tlabel, sizeof(tlabel));
        printf("[vk] post audio: %s + %s -> %s\n", audio, d->source, tlabel);
        fflush(stdout);
        VTL_AppResult rc = hub->pub->audio(audio, d->source, d->markup, d->targets);
        snprintf(reply, sizeof(reply), "Аудио: %s (код %d)",
                 rc == VTL_res_kOk ? "опубликовано" : "ошибка", (int)rc);
        vk_say(hub, peer_id, reply);

    } else if (!strcmp(cmd, "me")) {
        snprintf(reply, sizeof(reply), "peer_id: %lld", peer_id);
        vk_say(hub, peer_id, reply);

    } else if (!strcmp(cmd, "ping")) {
        vk_say(hub, peer_id, "pong");

    } else {
        vk_say(hub, peer_id, "Не знаю такой команды. /help — список.");
    }
}
