#include <VTL/bot/reminder/VTL_bot_reminder.h>
#include <VTL/bot/api/VTL_bot_api.h>

#include <stdio.h>
#include <string.h>
#include <time.h>

/* Контекст для колбэка: нужен токен, чтобы отправить сообщение. */
typedef struct {
    const char* token;
} VTL_bot_reminder_Ctx;

static void VTL_bot_reminder_OnDue(const char* chat_id, const char* text,
                                   void* user_data)
{
    VTL_bot_reminder_Ctx* ctx = (VTL_bot_reminder_Ctx*)user_data;
    char msg[1100];
    snprintf(msg, sizeof(msg), "\xe2\x8f\xb0 Напоминание: %s", text); /* ⏰ */
    VTL_bot_api_SendMessage(ctx->token, chat_id, msg);
}

int VTL_bot_reminder_Tick(VTL_bot_Store* store, const char* token)
{
    if (!store || !token) return 0;
    VTL_bot_reminder_Ctx ctx = { token };
    return VTL_bot_store_FireDueReminders(store, time(NULL),
                                          VTL_bot_reminder_OnDue, &ctx);
}
