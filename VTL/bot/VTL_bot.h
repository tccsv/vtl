#ifndef _VTL_BOT_H
#define _VTL_BOT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/VTL_app_result.h>

/* Запускает цикл бота (long-polling + напоминания) и блокирует поток до Ctrl+C.
 * store_path == NULL → VTL_BOT_DEFAULT_STORE. */
VTL_AppResult VTL_bot_Run(const char* token, const char* store_path);

#ifdef __cplusplus
}
#endif

#endif /* _VTL_BOT_H */
