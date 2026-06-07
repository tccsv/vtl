/*
 * main_firyulin.c — standalone-запуск Telegram-бота (модуль VTL/bot).
 *   cmake --build build --target main_firyulin
 */

#include <VTL/bot/VTL_bot.h>
#include <VTL/bot/VTL_bot_data.h>

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    /* stdout без буферизации — вывод сразу. _IONBF, не _IOLBF: size 0 валиден на MSVC. */
    setvbuf(stdout, NULL, _IONBF, 0);

    const char* token = getenv("TG_BOT_TOKEN");
    if (!token || !*token) {
        fprintf(stderr, "TG_BOT_TOKEN не задан.\n");
        return EXIT_FAILURE;
    }

    const char* store_path = (argc > 1) ? argv[1] : VTL_BOT_DEFAULT_STORE;

    printf("===========================================\n");
    printf("  VTL Telegram-бот: задачи + напоминания\n");
    printf("===========================================\n");

    return (VTL_bot_Run(token, store_path) == VTL_res_kOk)
               ? EXIT_SUCCESS : EXIT_FAILURE;
}
