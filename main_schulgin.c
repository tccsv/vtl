/*
 * main_schulgin.c — бот сообщества ВКонтакте как пульт публикации VTL.
 *
 * Принимает в диалоге выбор площадок и разметки и вызывает функции публикации
 * проекта. Здесь long-poll бота (модуль VTL/vkbot) связывается с реальными
 * VTL_PubicateMarkedText / VTL_PubicateAudioWithMarkedText.
 *
 * Переменные окружения: VK_TOKEN (ключ сообщества), VK_GROUP_ID (id сообщества).
 *
 *   cmake --build build --target main_schulgin
 */

#include <VTL/vkbot/VTL_vk_bot.h>
#include <VTL/publication/VTL_publication.h>

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);

    const char* token = getenv("VK_TOKEN");
    const char* group = getenv("VK_GROUP_ID");
    if (!token || !*token || !group || !*group) {
        fprintf(stderr, "Заданы не все переменные: нужны VK_TOKEN и VK_GROUP_ID.\n");
        return EXIT_FAILURE;
    }

    VTL_vk_Publishers pub;
    pub.text  = VTL_PubicateMarkedText;
    pub.audio = VTL_PubicateAudioWithMarkedText;

    printf("-------------------------------------------\n");
    printf("  VTL VK-бот: публикация по командам\n");
    printf("  /post -> VTL_PubicateMarkedText\n");
    printf("-------------------------------------------\n");

    return (VTL_vk_bot_Serve(token, group, &pub) == VTL_res_kOk)
               ? EXIT_SUCCESS : EXIT_FAILURE;
}
