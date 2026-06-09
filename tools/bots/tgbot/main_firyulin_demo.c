/* main_firyulin_demo.c — демо TG-бота без реальной публикации (заглушки).
 * С TG_BOT_TOKEN — Telegram, без — локальный REPL из stdin. */

#include <VTL/bots/tgbot/VTL_tgbot.h>
#include <VTL/bots/tgbot/command/VTL_tgbot_command.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static VTL_AppResult demo_PublishText(const VTL_Filename file_name,
                                      const VTL_content_platform_flags flags,
                                      const VTL_publication_marked_text_MarkupType markup)
{
    printf("    [server] VTL_PubicateMarkedText(file=\"%s\", flags=0x%X, markup=%d) -> OK\n",
           file_name, (unsigned)flags, (int)markup);
    return VTL_res_kOk;
}

static VTL_AppResult demo_PublishAudio(const VTL_Filename audio_file_name,
                                       const VTL_Filename text_file_name,
                                       const VTL_publication_marked_text_MarkupType markup,
                                       const VTL_content_platform_flags flags)
{
    printf("    [server] VTL_PubicateAudioWithMarkedText(audio=\"%s\", text=\"%s\", markup=%d, flags=0x%X) -> OK\n",
           audio_file_name, text_file_name, (int)markup, (unsigned)flags);
    return VTL_res_kOk;
}

static void demo_ConsoleReply(void* sink_ud, const char* chat_id, const char* text)
{
    (void)sink_ud; (void)chat_id;
    printf("БОТ> %s\n", text);
}

static int RunLocalRepl(const VTL_tgbot_Handlers* handlers)
{
    VTL_tgbot_sessionTable sessions;
    VTL_tgbot_session_TableInit(&sessions);

    VTL_tgbot_Context ctx;
    ctx.sessions = &sessions;
    ctx.handlers = handlers;
    ctx.reply    = demo_ConsoleReply;
    ctx.reply_ud = NULL;

    printf("=== VTL bot — локальный режим (без Telegram/FFmpeg) ===\n");
    printf("Вводи команды как в чат, напр.: /help, /platform tg w, /format asciidoc, /publish\n");
    printf("Пустая строка/EOF (Ctrl+Z, Enter) — выход.\n\n");

    char line[1024];
    while (fgets(line, sizeof(line), stdin)) {
        size_t n = strlen(line);
        while (n && (line[n - 1] == '\n' || line[n - 1] == '\r')) line[--n] = '\0';
        if (n == 0) break;
        printf("ВЫ > %s\n", line);
        VTL_tgbot_command_Handle(&ctx, "local", line);
    }
    printf("\n[demo] выход.\n");
    return 0;
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);

    VTL_tgbot_Handlers handlers;
    handlers.publish_text  = demo_PublishText;
    handlers.publish_audio = demo_PublishAudio;

    const char* token = getenv("TG_BOT_TOKEN");
    if (token && *token) {
        printf("=== VTL bot — демо в Telegram (публикация замокана) ===\n");
        return (VTL_tgbot_Run(token, &handlers) == VTL_res_kOk)
                   ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    return RunLocalRepl(&handlers);
}
