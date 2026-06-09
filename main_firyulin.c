/*
 * main_firyulin.c — Telegram-бот как пульт публикации VTL.
 *
 * Бот сам ничего не публикует: он принимает выбор платформ и формата разметки
 * и дёргает функции основного проекта. Здесь generic-цикл бота (модуль VTL/bot)
 * связывается с реальными точками входа VTL — VTL_PubicateMarkedText и
 * VTL_PubicateAudioWithMarkedText.
 *
 *   cmake --build build --target main_firyulin
 */

#include <VTL/bot/VTL_bot.h>
#include <VTL/publication/VTL_publication.h>

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    /* stdout без буферизации — вывод сразу. _IONBF, не _IOLBF: size 0 валиден на MSVC. */
    setvbuf(stdout, NULL, _IONBF, 0);

    const char* token = getenv("TG_BOT_TOKEN");
    if (!token || !*token) {
        fprintf(stderr, "TG_BOT_TOKEN не задан.\n");
        return EXIT_FAILURE;
    }

    /* Точки входа в функционал проекта: бот вызовет их по командам
     * /publish и /publish_audio с выбранными пользователем флагами. */
    VTL_bot_Handlers handlers;
    handlers.publish_text  = VTL_PubicateMarkedText;
    handlers.publish_audio = VTL_PubicateAudioWithMarkedText;

    printf("===========================================\n");
    printf("  VTL Telegram-бот: пульт публикации\n");
    printf("  /publish -> VTL_PubicateMarkedText\n");
    printf("===========================================\n");

    return (VTL_bot_Run(token, &handlers) == VTL_res_kOk)
               ? EXIT_SUCCESS : EXIT_FAILURE;
}
