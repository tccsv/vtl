/* main_firyulin.c — Telegram-бот, пульт публикации VTL. */

#include <VTL/bots/tgbot/VTL_tgbot.h>
#include <VTL/publication/VTL_publication.h>

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);

    const char* token = getenv("TG_BOT_TOKEN");
    if (!token || !*token) {
        fprintf(stderr, "TG_BOT_TOKEN не задан.\n");
        return EXIT_FAILURE;
    }

    VTL_tgbot_Handlers handlers;
    handlers.publish_text  = VTL_PubicateMarkedText;
    handlers.publish_audio = VTL_PubicateAudioWithMarkedText;

    printf("===========================================\n");
    printf("  VTL Telegram-бот: пульт публикации\n");
    printf("  /publish -> VTL_PubicateMarkedText\n");
    printf("===========================================\n");

    return (VTL_tgbot_Run(token, &handlers) == VTL_res_kOk)
               ? EXIT_SUCCESS : EXIT_FAILURE;
}
