#include <VTL/bots/vkbot/command/VTL_vk_command.h>
#include <VTL/user/history/db/VTL_user_history_db_save.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>

/* Отправка text в peer. */
static void vk_say(VTL_vk_Hub* hub, long long peer, const char* text)
{
    if (hub && hub->emit) hub->emit(hub->emit_ud, peer, text);
}

/* Файл открывается на чтение. */
static int vk_file_ok(const char* path)
{
    FILE* f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

static const char* vk_result_text(VTL_AppResult res)
{
    return (res == VTL_res_kOk) ? "OK" : "ОШИБКА";
}

/* Первое слово в cmd (нижний регистр, без '/'); возврат — хвост. */
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

/* Первый токен из args в tok. */
static void vk_first(const char* args, char* tok, size_t cap)
{
    size_t n = 0;
    while (args[n] && args[n] != ' ' && args[n] != '\t' && n + 1 < cap) { tok[n] = args[n]; ++n; }
    tok[n] = '\0';
}

static const char VK_HELP[] =
    "Бот публикации VTL (сообщество ВК).\n"
    "Выбираешь площадки и разметку — командой /post запускаешь публикацию.\n\n"
    "/targets <список> — площадки: vk, w, tg, reddit, vimeo, yt\n"
    "/markup <тип> — разметка: md, telegram, html, bb, asciidoc, mediawiki\n"
    "/source <файл> — файл с текстом\n"
    "/token <значение> — токен чата (включает запись публикаций в историю; off — сброс)\n"
    "/show — что выбрано сейчас\n"
    "/post — опубликовать текст\n"
    "/post_audio <файл> — опубликовать аудио с подписью\n"
    "/platforms — список площадок\n"
    "/formats — список форматов\n"
    "/me — мой peer_id\n"
    "/ping — проверка";

/* Запись публикации в историю (при заданном токене). */
static void vk_commit_text_to_history(const VTL_vk_Dialog* d)
{
    VTL_AppResult res = VTL_user_history_SaveTextPublication(d->source, d->targets);
    printf("[vk] history: commit text peer=%lld file=%s -> %s (код %d)\n",
           d->peer_id, d->source, vk_result_text(res), (int)res);
    fflush(stdout);
}

static void vk_commit_media_to_history(const VTL_vk_Dialog* d, const char* media)
{
    VTL_AppResult res = VTL_user_history_SaveMediaWTextPublication(d->source, media, d->targets);
    printf("[vk] history: commit media peer=%lld media=%s text=%s -> %s (код %d)\n",
           d->peer_id, media, d->source, vk_result_text(res), (int)res);
    fflush(stdout);
}

void VTL_vk_command_Help(VTL_vk_Hub* hub, VTL_vk_Dialog* d, long long peer_id, const char* args)
{
    (void)d; (void)args;
    vk_say(hub, peer_id, VK_HELP);
}

void VTL_vk_command_Targets(VTL_vk_Hub* hub, VTL_vk_Dialog* d, long long peer_id, const char* args)
{
    char reply[VTL_VK_REPLY_MAX];
    char tlabel[160];

    if (!*args) {
        VTL_vk_TargetsLabel(d->targets, tlabel, sizeof(tlabel));
        snprintf(reply, sizeof(reply), "Площадки: %s. Доступно: %s", tlabel, VTL_vk_TargetsHelp());
        vk_say(hub, peer_id, reply);
        return;
    }

    VTL_content_platform_flags acc = 0;
    char tok[32];
    const char* p = args;
    while (*p) {
        vk_first(p, tok, sizeof(tok));
        VTL_content_platform_flags b;
        if (*tok && VTL_vk_ParseTarget(tok, &b)) acc |= b;
        while (*p && *p != ' ') ++p;
        while (*p == ' ') ++p;
    }
    if (!acc) { vk_say(hub, peer_id, "Не понял ни одной площадки."); return; }

    VTL_vk_session_SetTargets(d, acc);
    VTL_vk_TargetsLabel(d->targets, tlabel, sizeof(tlabel));
    snprintf(reply, sizeof(reply), "Ок, площадки: %s", tlabel);
    vk_say(hub, peer_id, reply);
}

void VTL_vk_command_Markup(VTL_vk_Hub* hub, VTL_vk_Dialog* d, long long peer_id, const char* args)
{
    char reply[VTL_VK_REPLY_MAX];
    char tok[32];
    vk_first(args, tok, sizeof(tok));

    if (!*tok) {
        snprintf(reply, sizeof(reply), "Разметка: %s. Доступно: %s",
                 VTL_vk_MarkupLabel(d->markup), VTL_vk_MarkupsHelp());
        vk_say(hub, peer_id, reply);
        return;
    }
    VTL_publication_marked_text_MarkupType m;
    if (!VTL_vk_ParseMarkup(tok, &m)) { vk_say(hub, peer_id, "Такой разметки нет."); return; }
    VTL_vk_session_SetMarkup(d, m);
    snprintf(reply, sizeof(reply), "Разметка: %s", VTL_vk_MarkupLabel(m));
    vk_say(hub, peer_id, reply);
}

void VTL_vk_command_Source(VTL_vk_Hub* hub, VTL_vk_Dialog* d, long long peer_id, const char* args)
{
    char reply[VTL_VK_REPLY_MAX];
    char tok[VTL_VK_PATH_MAX];
    vk_first(args, tok, sizeof(tok));

    if (!*tok) { snprintf(reply, sizeof(reply), "Файл: %s", d->source); vk_say(hub, peer_id, reply); return; }
    VTL_vk_session_SetSource(d, tok);
    snprintf(reply, sizeof(reply), "Файл: %s", d->source);
    vk_say(hub, peer_id, reply);
}

void VTL_vk_command_Show(VTL_vk_Hub* hub, VTL_vk_Dialog* d, long long peer_id, const char* args)
{
    (void)args;
    char reply[VTL_VK_REPLY_MAX];
    char tlabel[160];
    VTL_vk_TargetsLabel(d->targets, tlabel, sizeof(tlabel));
    snprintf(reply, sizeof(reply), "Файл: %s\nПлощадки: %s\nРазметка: %s\nТокен: %s",
             d->source, tlabel, VTL_vk_MarkupLabel(d->markup),
             VTL_vk_session_HasToken(d) ? "задан" : "нет");
    vk_say(hub, peer_id, reply);
}

void VTL_vk_command_Token(VTL_vk_Hub* hub, VTL_vk_Dialog* d, long long peer_id, const char* args)
{
    char tok[VTL_VK_TOKEN_MAX];
    vk_first(args, tok, sizeof(tok));

    if (!*tok) {
        vk_say(hub, peer_id,
            VTL_vk_session_HasToken(d)
                ? "Токен задан (запись в историю включена).\n"
                  "Сменить: /token <значение>\nСбросить: /token off"
                : "Токен не задан.\nЗадать: /token <значение>");
        return;
    }
    if (!strcmp(tok, "off")) {
        VTL_vk_session_ClearToken(d);
        vk_say(hub, peer_id, "Токен сброшен — запись в историю отключена.");
        return;
    }
    VTL_vk_session_SetToken(d, tok);
    vk_say(hub, peer_id, "Токен сохранён — публикации будут писаться в историю/БД.");
}

void VTL_vk_command_Post(VTL_vk_Hub* hub, VTL_vk_Dialog* d, long long peer_id, const char* args)
{
    (void)args;
    char reply[VTL_VK_REPLY_MAX];
    char tlabel[160];

    if (!hub->pub || !hub->pub->text) { vk_say(hub, peer_id, "Публикация текста недоступна."); return; }
    if (!vk_file_ok(d->source)) {
        snprintf(reply, sizeof(reply), "Нет файла: %s (задай /source)", d->source);
        vk_say(hub, peer_id, reply);
        return;
    }

    VTL_vk_TargetsLabel(d->targets, tlabel, sizeof(tlabel));
    printf("[vk] post text: %s -> %s (%s)\n", d->source, tlabel, VTL_vk_MarkupLabel(d->markup));
    fflush(stdout);

    VTL_AppResult rc = hub->pub->text(d->source, d->targets, d->markup);
    if (VTL_vk_session_HasToken(d)) vk_commit_text_to_history(d);

    snprintf(reply, sizeof(reply), "Текст: %s (код %d) [%s, %s]",
             rc == VTL_res_kOk ? "опубликовано" : "ошибка", (int)rc,
             tlabel, VTL_vk_MarkupLabel(d->markup));
    vk_say(hub, peer_id, reply);
}

void VTL_vk_command_PostAudio(VTL_vk_Hub* hub, VTL_vk_Dialog* d, long long peer_id, const char* args)
{
    char reply[VTL_VK_REPLY_MAX];
    char tlabel[160];
    char audio[VTL_VK_PATH_MAX];
    vk_first(args, audio, sizeof(audio));

    if (!*audio) { vk_say(hub, peer_id, "Укажи аудиофайл: /post_audio file.mp3"); return; }
    if (!hub->pub || !hub->pub->audio) { vk_say(hub, peer_id, "Публикация аудио недоступна."); return; }
    if (!vk_file_ok(audio))     { snprintf(reply, sizeof(reply), "Нет аудио: %s", audio); vk_say(hub, peer_id, reply); return; }
    if (!vk_file_ok(d->source)) { snprintf(reply, sizeof(reply), "Нет файла текста: %s", d->source); vk_say(hub, peer_id, reply); return; }

    VTL_vk_TargetsLabel(d->targets, tlabel, sizeof(tlabel));
    printf("[vk] post audio: %s + %s -> %s\n", audio, d->source, tlabel);
    fflush(stdout);

    VTL_AppResult rc = hub->pub->audio(audio, d->source, d->markup, d->targets);
    if (VTL_vk_session_HasToken(d)) vk_commit_media_to_history(d, audio);

    snprintf(reply, sizeof(reply), "Аудио: %s (код %d)",
             rc == VTL_res_kOk ? "опубликовано" : "ошибка", (int)rc);
    vk_say(hub, peer_id, reply);
}

void VTL_vk_command_TargetsList(VTL_vk_Hub* hub, VTL_vk_Dialog* d, long long peer_id, const char* args)
{
    (void)d; (void)args;
    char reply[VTL_VK_REPLY_MAX];
    snprintf(reply, sizeof(reply), "Площадки: %s", VTL_vk_TargetsHelp());
    vk_say(hub, peer_id, reply);
}

void VTL_vk_command_MarkupsList(VTL_vk_Hub* hub, VTL_vk_Dialog* d, long long peer_id, const char* args)
{
    (void)d; (void)args;
    char reply[VTL_VK_REPLY_MAX];
    snprintf(reply, sizeof(reply), "Форматы разметки: %s", VTL_vk_MarkupsHelp());
    vk_say(hub, peer_id, reply);
}

void VTL_vk_command_Me(VTL_vk_Hub* hub, VTL_vk_Dialog* d, long long peer_id, const char* args)
{
    (void)d; (void)args;
    char reply[64];
    snprintf(reply, sizeof(reply), "peer_id: %lld", peer_id);
    vk_say(hub, peer_id, reply);
}

void VTL_vk_command_Ping(VTL_vk_Hub* hub, VTL_vk_Dialog* d, long long peer_id, const char* args)
{
    (void)d; (void)args;
    vk_say(hub, peer_id, "pong");
}

typedef void (*VTL_vk_command_Fn)(VTL_vk_Hub*, VTL_vk_Dialog*, long long, const char*);

static const struct { const char* name; VTL_vk_command_Fn fn; } VTL_vk_command_kTable[] = {
    { "help",          VTL_vk_command_Help        },
    { "start",         VTL_vk_command_Help        },
    { "targets",       VTL_vk_command_Targets     },
    { "markup",        VTL_vk_command_Markup      },
    { "source",        VTL_vk_command_Source      },
    { "show",          VTL_vk_command_Show        },
    { "status",        VTL_vk_command_Show        },
    { "token",         VTL_vk_command_Token       },
    { "tocken",        VTL_vk_command_Token       },
    { "post",          VTL_vk_command_Post        },
    { "publish",       VTL_vk_command_Post        },
    { "post_audio",    VTL_vk_command_PostAudio   },
    { "postaudio",     VTL_vk_command_PostAudio   },
    { "publish_audio", VTL_vk_command_PostAudio   },
    { "platforms",     VTL_vk_command_TargetsList },
    { "targets_list",  VTL_vk_command_TargetsList },
    { "formats",       VTL_vk_command_MarkupsList },
    { "markups",       VTL_vk_command_MarkupsList },
    { "me",            VTL_vk_command_Me          },
    { "id",            VTL_vk_command_Me          },
    { "ping",          VTL_vk_command_Ping        },
};

void VTL_vk_hub_Handle(VTL_vk_Hub* hub, long long peer_id, const char* text)
{
    if (!hub || !text) return;

    VTL_vk_Dialog* d = VTL_vk_hub_Dialog(hub, peer_id);
    if (!d) { vk_say(hub, peer_id, "Слишком много диалогов, позже."); return; }

    char cmd[32];
    const char* args = vk_split(text, cmd, sizeof(cmd));

    if (!cmd[0]) { VTL_vk_command_Help(hub, d, peer_id, args); return; }

    const size_t count = sizeof(VTL_vk_command_kTable) / sizeof(VTL_vk_command_kTable[0]);
    for (size_t i = 0; i < count; ++i) {
        if (!strcmp(cmd, VTL_vk_command_kTable[i].name)) {
            VTL_vk_command_kTable[i].fn(hub, d, peer_id, args);
            return;
        }
    }
    vk_say(hub, peer_id, "Не знаю такой команды. /help — список.");
}
