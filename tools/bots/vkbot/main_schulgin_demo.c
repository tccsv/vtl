/* main_schulgin_demo.c — демо VK-бота без реальной публикации (заглушки).
 * С VK_TOKEN+VK_GROUP_ID — long-poll, иначе локальный разбор из stdin. */

#include <VTL/bots/vkbot/VTL_vk_bot.h>
#include <VTL/bots/vkbot/session/VTL_vk_session.h>
#include <VTL/bots/vkbot/command/VTL_vk_command.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static VTL_AppResult stub_text(const VTL_Filename file,
                               const VTL_content_platform_flags flags,
                               const VTL_publication_marked_text_MarkupType markup)
{
    printf("    [mock] post text  file=%s flags=0x%X markup=%d -> ok\n",
           file, (unsigned)flags, (int)markup);
    return VTL_res_kOk;
}

static VTL_AppResult stub_audio(const VTL_Filename audio, const VTL_Filename text,
                                const VTL_publication_marked_text_MarkupType markup,
                                const VTL_content_platform_flags flags)
{
    printf("    [mock] post audio audio=%s text=%s markup=%d flags=0x%X -> ok\n",
           audio, text, (int)markup, (unsigned)flags);
    return VTL_res_kOk;
}

static void console_emit(void* ud, long long peer_id, const char* text)
{
    (void)ud; (void)peer_id;
    printf("<bot> %s\n", text);
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);

    VTL_vk_Publishers pub;
    pub.text  = stub_text;
    pub.audio = stub_audio;

    const char* token = getenv("VK_TOKEN");
    const char* group = getenv("VK_GROUP_ID");
    if (token && *token && group && *group) {
        printf("== VK-бот, демо-режим (публикация замокирована) ==\n");
        return (VTL_vk_bot_Serve(token, group, &pub) == VTL_res_kOk)
                   ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    VTL_vk_Hub hub;
    VTL_vk_hub_Reset(&hub, &pub, console_emit, NULL);
    printf("== VK-бот, локальный разбор (без VK/FFmpeg) ==\n");
    printf("Команды как в диалоге: /help, /targets vk w, /markup html, /post ...\n");
    printf("Пустая строка — выход.\n\n");

    char line[1024];
    while (fgets(line, sizeof(line), stdin)) {
        size_t n = strlen(line);
        while (n && (line[n-1] == '\n' || line[n-1] == '\r')) line[--n] = '\0';
        if (!n) break;
        printf("you> %s\n", line);
        VTL_vk_hub_Handle(&hub, 1, line);
    }
    return 0;
}
