#ifndef _VTL_VKBOT_H
#define _VTL_VKBOT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/VTL_app_result.h>

/* Запускает цикл VK-бота (Bots Long Poll) и блокирует поток до Ctrl+C.
 * token     — ключ доступа сообщества (community access token);
 * group_id  — числовой id сообщества (без минуса);
 * store_path == NULL → VTL_VKBOT_DEFAULT_STORE. */
VTL_AppResult VTL_vkbot_Run(const char* token, const char* group_id,
                            const char* store_path);

#ifdef __cplusplus
}
#endif

#endif /* _VTL_VKBOT_H */
