/*
 * main_schulgin.c — standalone-запуск VK-бота личных финансов (модуль VTL/vkbot).
 *   cmake --build build --target main_schulgin
 *
 * Переменные окружения:
 *   VK_BOT_TOKEN — ключ доступа сообщества (community access token);
 *   VK_GROUP_ID  — числовой id сообщества (без минуса).
 * Аргумент: путь к файлу хранилища (по умолчанию VTL_VKBOT_DEFAULT_STORE).
 */

#include <VTL/vkbot/VTL_vkbot.h>
#include <VTL/vkbot/VTL_vkbot_data.h>

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    /* stdout без буферизации — вывод сразу. _IONBF, не _IOLBF: size 0 валиден на MSVC. */
    setvbuf(stdout, NULL, _IONBF, 0);

    const char* token = getenv("VK_BOT_TOKEN");
    if (!token || !*token) {
        fprintf(stderr, "VK_BOT_TOKEN не задан.\n");
        return EXIT_FAILURE;
    }

    const char* group_id = getenv("VK_GROUP_ID");
    if (!group_id || !*group_id) {
        fprintf(stderr, "VK_GROUP_ID не задан.\n");
        return EXIT_FAILURE;
    }

    const char* store_path = (argc > 1) ? argv[1] : VTL_VKBOT_DEFAULT_STORE;

    printf("===========================================\n");
    printf("  VTL VK-бот: личные финансы\n");
    printf("===========================================\n");

    return (VTL_vkbot_Run(token, group_id, store_path) == VTL_res_kOk)
               ? EXIT_SUCCESS : EXIT_FAILURE;
}
