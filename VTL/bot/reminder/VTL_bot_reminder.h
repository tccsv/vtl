#ifndef _VTL_BOT_REMINDER_H
#define _VTL_BOT_REMINDER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/bot/store/VTL_bot_store.h>

/* Рассылает наступившие напоминания. Возвращает число отправленных. */
int VTL_bot_reminder_Tick(VTL_bot_Store* store, const char* token);

#ifdef __cplusplus
}
#endif

#endif /* _VTL_BOT_REMINDER_H */
