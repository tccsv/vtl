#include <VTL/bots/tgbot/command/VTL_tgbot_command.h>
#include <VTL/bots/tgbot/VTL_tgbot_data.h>
#include <VTL/user/history/db/VTL_user_history_db_save.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* VTL_tgbot_command_kHelp =
    "Я бот-пульт публикации VTL. Через меня вы выбираете платформы и формат\n"
    "разметки и запускаете публикацию текста/аудио функциями проекта.\n"
    "\n"
    "Параметры публикации:\n"
    "/platform <список> — выбрать платформы (tg, w, reddit, vimeo, yt, vk)\n"
    "/format <имя> — формат разметки (md, telegram, html, bb, asciidoc, mediawiki)\n"
    "/file <путь> — файл текста (по умолчанию text.md)\n"
    "/token <значение> — токен чата (включает запись публикаций в БД; off — сброс)\n"
    "/status — текущий выбор\n"
    "\n"
    "Запуск (вызов функций сервера VTL):\n"
    "/publish — VTL_PubicateMarkedText(файл, платформы, формат)\n"
    "/publish_audio <аудио> — VTL_PubicateAudioWithMarkedText(аудио, файл, формат, платформы)\n"
    "\n"
    "Сервис:\n"
    "/platforms — список платформ\n"
    "/formats — список форматов\n"
    "/id — мой chat_id\n"
    "/ping — проверка связи\n"
    "/help — эта справка";

static const char* VTL_tgbot_command_Skip(const char* s)
{
    while (*s && isspace((unsigned char)*s)) ++s;
    return s;
}

/* Копирует первый токен из s в tok, возвращает остаток. */
static const char* VTL_tgbot_command_NextToken(const char* s, char* tok, size_t cap)
{
    s = VTL_tgbot_command_Skip(s);
    size_t n = 0;
    while (s[n] && !isspace((unsigned char)s[n]) && n + 1 < cap) {
        tok[n] = s[n];
        ++n;
    }
    tok[n] = '\0';
    return VTL_tgbot_command_Skip(s + n);
}

static void VTL_tgbot_command_Reply(VTL_tgbot_Context* ctx, const char* chat_id,
                                  const char* text)
{
    if (ctx->reply) ctx->reply(ctx->reply_ud, chat_id, text);
}

static const char* VTL_tgbot_command_ResultText(VTL_AppResult res)
{
    return (res == VTL_res_kOk) ? "OK" : "ОШИБКА";
}

/* Проверяет, что файл можно открыть на чтение. */
static int VTL_tgbot_command_FileReadable(const char* path)
{
    FILE* f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}


/* Пишет публикацию текста в историю (БД). */
static void VTL_tgbot_command_CommitTextToHistory(const VTL_tgbot_session* s)
{
    VTL_AppResult res = VTL_user_history_SaveTextPublication(s->text_file, s->platforms);
    printf("[bot] history: commit text chat=%s file=%s -> %s (код %d)\n",
           s->chat_id, s->text_file, VTL_tgbot_command_ResultText(res), (int)res);
    fflush(stdout);
}

/* Пишет публикацию аудио + текст в историю (БД). */
static void VTL_tgbot_command_CommitMediaToHistory(const VTL_tgbot_session* s,
                                                 const char* media)
{
    VTL_AppResult res = VTL_user_history_SaveMediaWTextPublication(
            s->text_file, media, s->platforms);
    printf("[bot] history: commit media chat=%s media=%s text=%s -> %s (код %d)\n",
           s->chat_id, media, s->text_file, VTL_tgbot_command_ResultText(res), (int)res);
    fflush(stdout);
}

typedef void (*VTL_tgbot_command_Fn)(VTL_tgbot_Context*, VTL_tgbot_session*,
                                   const char* chat_id, const char* arg);

/* /platform <список> — выбрать платформы. */
static void VTL_tgbot_command_HandlePlatform(VTL_tgbot_Context* ctx, VTL_tgbot_session* s,
                                           const char* chat_id, const char* arg)
{
    char reply[VTL_tgbot_REPLY_MAX];
    char list[256];

    if (!*arg) {
        VTL_tgbot_session_DescribePlatforms(s->platforms, list, sizeof(list));
        snprintf(reply, sizeof(reply),
                 "Сейчас выбрано: %s\nДоступно: %s\nЗадать: /platform tg w reddit",
                 list, VTL_tgbot_session_PlatformsHelp());
        VTL_tgbot_command_Reply(ctx, chat_id, reply);
        return;
    }

    VTL_content_platform_flags flags = 0;
    char tok[64];
    int  bad = 0;
    char bad_names[160];
    bad_names[0] = '\0';

    const char* rest = arg;
    while (*rest) {
        rest = VTL_tgbot_command_NextToken(rest, tok, sizeof(tok));
        if (!*tok) break;
        VTL_content_platform_flags bit = 0;
        if (VTL_tgbot_session_ParsePlatform(tok, &bit)) {
            flags |= bit;
        } else {
            ++bad;
            size_t used = strlen(bad_names);
            snprintf(bad_names + used, sizeof(bad_names) - used,
                     "%s%s", used ? ", " : "", tok);
        }
    }

    if (flags == 0) {
        snprintf(reply, sizeof(reply),
                 "Не распознал ни одной платформы (%s). Доступно: %s",
                 bad_names, VTL_tgbot_session_PlatformsHelp());
        VTL_tgbot_command_Reply(ctx, chat_id, reply);
        return;
    }

    s->platforms = flags;
    VTL_tgbot_session_DescribePlatforms(s->platforms, list, sizeof(list));
    if (bad) {
        snprintf(reply, sizeof(reply),
                 "Платформы: %s (пропущено: %s).", list, bad_names);
    } else {
        snprintf(reply, sizeof(reply), "Платформы: %s.", list);
    }
    VTL_tgbot_command_Reply(ctx, chat_id, reply);
}

/* /format <имя> — выбрать формат разметки. */
static void VTL_tgbot_command_HandleFormat(VTL_tgbot_Context* ctx, VTL_tgbot_session* s,
                                         const char* chat_id, const char* arg)
{
    char reply[VTL_tgbot_REPLY_MAX];
    char tok[64];
    VTL_tgbot_command_NextToken(arg, tok, sizeof(tok));

    if (!*tok) {
        snprintf(reply, sizeof(reply),
                 "Сейчас формат: %s\nДоступно: %s\nЗадать: /format asciidoc",
                 VTL_tgbot_session_FormatName(s->markup),
                 VTL_tgbot_session_FormatsHelp());
        VTL_tgbot_command_Reply(ctx, chat_id, reply);
        return;
    }

    VTL_publication_marked_text_MarkupType markup;
    if (!VTL_tgbot_session_ParseFormat(tok, &markup)) {
        snprintf(reply, sizeof(reply),
                 "Неизвестный формат «%s». Доступно: %s",
                 tok, VTL_tgbot_session_FormatsHelp());
        VTL_tgbot_command_Reply(ctx, chat_id, reply);
        return;
    }

    s->markup = markup;
    snprintf(reply, sizeof(reply), "Формат: %s.",
             VTL_tgbot_session_FormatName(s->markup));
    VTL_tgbot_command_Reply(ctx, chat_id, reply);
}

/* /file <путь> — выбрать файл текста. */
static void VTL_tgbot_command_HandleFile(VTL_tgbot_Context* ctx, VTL_tgbot_session* s,
                                       const char* chat_id, const char* arg)
{
    char reply[VTL_tgbot_REPLY_MAX];
    char tok[VTL_tgbot_FILE_MAX];
    VTL_tgbot_command_NextToken(arg, tok, sizeof(tok));

    if (!*tok) {
        snprintf(reply, sizeof(reply),
                 "Сейчас файл: %s\nЗадать: /file text.adoc", s->text_file);
        VTL_tgbot_command_Reply(ctx, chat_id, reply);
        return;
    }

    snprintf(s->text_file, sizeof(s->text_file), "%s", tok);
    snprintf(reply, sizeof(reply), "Файл текста: %s.", s->text_file);
    VTL_tgbot_command_Reply(ctx, chat_id, reply);
}

/* /token <значение> */
static void VTL_tgbot_command_HandleToken(VTL_tgbot_Context* ctx, VTL_tgbot_session* s,
                                        const char* chat_id, const char* arg)
{
    char tok[VTL_tgbot_TOKEN_MAX];
    VTL_tgbot_command_NextToken(arg, tok, sizeof(tok));

    if (!*tok) {
        VTL_tgbot_command_Reply(ctx, chat_id,
            s->token[0] ? "Токен задан (запись в БД включена).\n"
                          "Сменить: /token <значение>\nСбросить: /token off"
                        : "Токен не задан.\nЗадать: /token <значение>");
        return;
    }

    if (strcmp(tok, "off") == 0) {
        s->token[0] = '\0';
        VTL_tgbot_command_Reply(ctx, chat_id,
            "Токен сброшен — запись в БД отключена.");
        return;
    }

    snprintf(s->token, sizeof(s->token), "%s", tok);
    VTL_tgbot_command_Reply(ctx, chat_id,
        "Токен сохранён — публикации будут писаться в БД.");
}

/* /status — показать текущий выбор. */
static void VTL_tgbot_command_HandleStatus(VTL_tgbot_Context* ctx, VTL_tgbot_session* s,
                                         const char* chat_id, const char* arg)
{
    (void)arg;
    char reply[VTL_tgbot_REPLY_MAX];
    char list[256];
    VTL_tgbot_session_DescribePlatforms(s->platforms, list, sizeof(list));
    snprintf(reply, sizeof(reply),
             "Параметры публикации:\n"
             "Файл: %s\nПлатформы: %s\nФормат: %s\n\n"
             "/publish — опубликовать текст.",
             s->text_file, list, VTL_tgbot_session_FormatName(s->markup));
    VTL_tgbot_command_Reply(ctx, chat_id, reply);
}

/* /publish — опубликовать текст. */
static void VTL_tgbot_command_HandlePublish(VTL_tgbot_Context* ctx, VTL_tgbot_session* s,
                                          const char* chat_id, const char* arg)
{
    (void)arg;
    char reply[VTL_tgbot_REPLY_MAX];
    char list[256];

    if (!ctx->handlers || !ctx->handlers->publish_text) {
        VTL_tgbot_command_Reply(ctx, chat_id,
            "Публикация текста недоступна (handler не подключён).");
        return;
    }
    if (!VTL_tgbot_command_FileReadable(s->text_file)) {
        snprintf(reply, sizeof(reply),
                 "Файл не найден: %s\nЗадай существующий: /file text.adoc", s->text_file);
        VTL_tgbot_command_Reply(ctx, chat_id, reply);
        return;
    }

    VTL_tgbot_session_DescribePlatforms(s->platforms, list, sizeof(list));
    printf("[bot] publish text: файл=%s платформы=%s формат=%s\n",
           s->text_file, list, VTL_tgbot_session_FormatName(s->markup));
    fflush(stdout);

    VTL_AppResult res = ctx->handlers->publish_text(
            s->text_file, s->platforms, s->markup);

    if (s->token[0]) VTL_tgbot_command_CommitTextToHistory(s);

    snprintf(reply, sizeof(reply),
             "Публикация текста: %s (код %d)\nФайл: %s\nПлатформы: %s\nФормат: %s",
             VTL_tgbot_command_ResultText(res), (int)res,
             s->text_file, list, VTL_tgbot_session_FormatName(s->markup));
    VTL_tgbot_command_Reply(ctx, chat_id, reply);
}

/* /publish_audio <аудио> — опубликовать аудио с текстом. */
static void VTL_tgbot_command_HandlePublishAudio(VTL_tgbot_Context* ctx, VTL_tgbot_session* s,
                                               const char* chat_id, const char* arg)
{
    char reply[VTL_tgbot_REPLY_MAX];
    char list[256];
    char audio[VTL_tgbot_FILE_MAX];
    VTL_tgbot_command_NextToken(arg, audio, sizeof(audio));

    if (!*audio) {
        VTL_tgbot_command_Reply(ctx, chat_id,
            "Укажи аудиофайл: /publish_audio audio_ariel.mp3");
        return;
    }
    if (!ctx->handlers || !ctx->handlers->publish_audio) {
        VTL_tgbot_command_Reply(ctx, chat_id,
            "Публикация аудио недоступна (handler не подключён).");
        return;
    }
    if (!VTL_tgbot_command_FileReadable(audio)) {
        snprintf(reply, sizeof(reply), "Аудиофайл не найден: %s", audio);
        VTL_tgbot_command_Reply(ctx, chat_id, reply);
        return;
    }
    if (!VTL_tgbot_command_FileReadable(s->text_file)) {
        snprintf(reply, sizeof(reply),
                 "Файл текста не найден: %s\nЗадай существующий: /file text.adoc", s->text_file);
        VTL_tgbot_command_Reply(ctx, chat_id, reply);
        return;
    }

    VTL_tgbot_session_DescribePlatforms(s->platforms, list, sizeof(list));
    printf("[bot] publish audio: аудио=%s текст=%s платформы=%s формат=%s\n",
           audio, s->text_file, list, VTL_tgbot_session_FormatName(s->markup));
    fflush(stdout);

    VTL_AppResult res = ctx->handlers->publish_audio(
            audio, s->text_file, s->markup, s->platforms);

    if (s->token[0]) VTL_tgbot_command_CommitMediaToHistory(s, audio);

    snprintf(reply, sizeof(reply),
             "Публикация аудио: %s (код %d)\nАудио: %s\nТекст: %s\nПлатформы: %s\nФормат: %s",
             VTL_tgbot_command_ResultText(res), (int)res,
             audio, s->text_file, list, VTL_tgbot_session_FormatName(s->markup));
    VTL_tgbot_command_Reply(ctx, chat_id, reply);
}

/* /platforms — список платформ. */
static void VTL_tgbot_command_HandlePlatforms(VTL_tgbot_Context* ctx, VTL_tgbot_session* s,
                                            const char* chat_id, const char* arg)
{
    (void)s; (void)arg;
    char reply[VTL_tgbot_REPLY_MAX];
    snprintf(reply, sizeof(reply), "Платформы: %s",
             VTL_tgbot_session_PlatformsHelp());
    VTL_tgbot_command_Reply(ctx, chat_id, reply);
}

/* /formats — список форматов. */
static void VTL_tgbot_command_HandleFormats(VTL_tgbot_Context* ctx, VTL_tgbot_session* s,
                                          const char* chat_id, const char* arg)
{
    (void)s; (void)arg;
    char reply[VTL_tgbot_REPLY_MAX];
    snprintf(reply, sizeof(reply), "Форматы разметки: %s",
             VTL_tgbot_session_FormatsHelp());
    VTL_tgbot_command_Reply(ctx, chat_id, reply);
}

/* /id — показать chat_id. */
static void VTL_tgbot_command_HandleId(VTL_tgbot_Context* ctx, VTL_tgbot_session* s,
                                     const char* chat_id, const char* arg)
{
    (void)s; (void)arg;
    char reply[VTL_tgbot_REPLY_MAX];
    snprintf(reply, sizeof(reply), "Твой chat_id: %s", chat_id);
    VTL_tgbot_command_Reply(ctx, chat_id, reply);
}

/* /ping — проверка связи. */
static void VTL_tgbot_command_HandlePing(VTL_tgbot_Context* ctx, VTL_tgbot_session* s,
                                       const char* chat_id, const char* arg)
{
    (void)s; (void)arg;
    VTL_tgbot_command_Reply(ctx, chat_id, "🏓 pong");
}

/* /start, /help — справка. */
static void VTL_tgbot_command_HandleHelp(VTL_tgbot_Context* ctx, VTL_tgbot_session* s,
                                       const char* chat_id, const char* arg)
{
    (void)s; (void)arg;
    VTL_tgbot_command_Reply(ctx, chat_id, VTL_tgbot_command_kHelp);
}

typedef struct
{
    const char*        name;
    VTL_tgbot_command_Fn fn;
} VTL_tgbot_command_Entry;

static const VTL_tgbot_command_Entry VTL_tgbot_command_kTable[] = {
    { "platform",      VTL_tgbot_command_HandlePlatform     },
    { "format",        VTL_tgbot_command_HandleFormat       },
    { "file",          VTL_tgbot_command_HandleFile         },
    { "status",        VTL_tgbot_command_HandleStatus       },
    { "token",         VTL_tgbot_command_HandleToken        },
    { "tocken",        VTL_tgbot_command_HandleToken        },
    { "publish",       VTL_tgbot_command_HandlePublish      },
    { "publish_audio", VTL_tgbot_command_HandlePublishAudio },
    { "publishaudio",  VTL_tgbot_command_HandlePublishAudio },
    { "platforms",     VTL_tgbot_command_HandlePlatforms    },
    { "formats",       VTL_tgbot_command_HandleFormats      },
    { "id",            VTL_tgbot_command_HandleId           },
    { "ping",          VTL_tgbot_command_HandlePing         },
    { "start",         VTL_tgbot_command_HandleHelp         },
    { "help",          VTL_tgbot_command_HandleHelp         },
};

/* Извлекает имя команды в cmd, возвращает аргументы после него. */
static const char* VTL_tgbot_command_ParseName(const char* text, char* cmd, size_t cap)
{
    size_t n = 0;
    while (text[n] && !isspace((unsigned char)text[n]) && text[n] != '@'
           && n + 1 < cap) {
        cmd[n] = (char)tolower((unsigned char)text[n]);
        ++n;
    }
    cmd[n] = '\0';

    const char* rest = text + n;
    while (*rest && *rest != ' ') ++rest;        /* пропускаем @botname */
    return VTL_tgbot_command_Skip(rest);
}

void VTL_tgbot_command_Handle(VTL_tgbot_Context* ctx, const char* chat_id,
                            const char* text)
{
    if (!ctx || !ctx->sessions || !ctx->reply || !chat_id) return;

    VTL_tgbot_session* s = VTL_tgbot_session_GetOrCreate(ctx->sessions, chat_id);
    if (!s) {
        VTL_tgbot_command_Reply(ctx, chat_id,
            "Слишком много активных чатов, попробуйте позже.");
        return;
    }

    text = text ? VTL_tgbot_command_Skip(text) : "";
    if (*text != '/') {
        VTL_tgbot_command_Reply(ctx, chat_id,
            "Не понял. Напиши /help, чтобы увидеть команды.");
        return;
    }

    char cmd[32];
    const char* arg = VTL_tgbot_command_ParseName(text + 1, cmd, sizeof(cmd));

    const size_t count = sizeof(VTL_tgbot_command_kTable) /
                         sizeof(VTL_tgbot_command_kTable[0]);
    for (size_t i = 0; i < count; ++i) {
        if (strcmp(cmd, VTL_tgbot_command_kTable[i].name) == 0) {
            VTL_tgbot_command_kTable[i].fn(ctx, s, chat_id, arg);
            return;
        }
    }

    VTL_tgbot_command_Reply(ctx, chat_id,
        "Неизвестная команда. /help — список команд.");
}
