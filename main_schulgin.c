/* main_schulgin.c — VK-бот сообщества, пульт публикации VTL.
 * Окружение: VK_TOKEN, VK_GROUP_ID. */

#include <VTL/bots/vkbot/VTL_vk_bot.h>
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
