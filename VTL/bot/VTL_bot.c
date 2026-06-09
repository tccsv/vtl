#include <VTL/bot/VTL_bot.h>
#include <VTL/bot/VTL_bot_data.h>
#include <VTL/bot/api/VTL_bot_api.h>
#include <VTL/bot/session/VTL_bot_session.h>
#include <VTL/bot/command/VTL_bot_command.h>

#include <parson/parson.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#  include <windows.h>
#  define VTL_BOT_SLEEP(sec) Sleep((unsigned long)(sec) * 1000)
#else
#  include <unistd.h>
#  define VTL_BOT_SLEEP(sec) sleep((unsigned int)(sec))
#endif

/* Флаг останова, выставляется обработчиком сигнала. */
static volatile sig_atomic_t VTL_bot_g_stop = 0;

static void VTL_bot_OnSignal(int sig)
{
    (void)sig;
    VTL_bot_g_stop = 1;
}

/* Sink ответов в проде: отправляет text в чат через Telegram Bot API.
 * sink_ud — токен бота. Ошибки транспорта только логируются. */
static void VTL_bot_TelegramReply(void* sink_ud, const char* chat_id,
                                  const char* text)
{
    const char* token = (const char*)sink_ud;
    if (VTL_bot_api_SendMessage(token, chat_id, text) != VTL_res_kOk)
        fprintf(stderr, "[bot] sendMessage в чат %s не удался\n", chat_id);
}

/* Разбирает getUpdates и обрабатывает апдейты. Возвращает offset для
 * следующего запроса (max update_id + 1). */
static long VTL_bot_ProcessUpdates(const char* body, VTL_bot_Context* ctx,
                                   long offset)
{
    JSON_Value* root = json_parse_string(body);
    if (!root) return offset;

    JSON_Object* obj = json_value_get_object(root);
    if (!obj || json_object_get_boolean(obj, "ok") <= 0) {
        json_value_free(root);
        return offset;
    }

    JSON_Array* updates = json_object_get_array(obj, "result");
    size_t count = updates ? json_array_get_count(updates) : 0;

    for (size_t i = 0; i < count; ++i) {
        JSON_Object* upd = json_array_get_object(updates, i);
        long update_id = (long)json_object_get_number(upd, "update_id");
        if (update_id >= offset) offset = update_id + 1;

        /* Берём только обычные текстовые сообщения. */
        JSON_Object* msg = json_object_get_object(upd, "message");
        if (!msg) continue;
        const char* text = json_object_get_string(msg, "text");
        if (!text) continue;

        double chat_num = json_object_dotget_number(msg, "chat.id");
        if (chat_num == 0) continue;
        char chat_id[32];
        snprintf(chat_id, sizeof(chat_id), "%lld", (long long)chat_num);

        printf("[bot] <- chat %s: %s\n", chat_id, text);
        fflush(stdout);
        VTL_bot_command_Handle(ctx, chat_id, text);
    }

    json_value_free(root);
    return offset;
}

VTL_AppResult VTL_bot_Run(const char* token, const VTL_bot_Handlers* handlers)
{
    if (!token || !*token) return VTL_res_kInvalidParamErr;

    VTL_bot_SessionTable sessions;
    VTL_bot_session_TableInit(&sessions);

    VTL_bot_Context ctx;
    ctx.sessions = &sessions;
    ctx.handlers = handlers;
    ctx.reply    = VTL_bot_TelegramReply;
    ctx.reply_ud = (void*)token;

    signal(SIGINT, VTL_bot_OnSignal);
#ifdef SIGTERM
    signal(SIGTERM, VTL_bot_OnSignal);
#endif

    printf("[bot] запущен. Ctrl+C для остановки.\n");
    fflush(stdout); /* в составе VTL stdout блочно-буферизован — форсим вывод */

    char bot_name[64];
    if (VTL_bot_api_GetMe(token, bot_name, sizeof(bot_name))) {
        printf("[bot] подключён как @%s\n", bot_name);
        fflush(stdout);
    }

    /* Сброс webhook: иначе long-polling getUpdates даёт HTTP 409 (Conflict). */
    if (VTL_bot_api_DeleteWebhook(token)) {
        printf("[bot] webhook сброшен (long-polling готов).\n");
        fflush(stdout);
    }

    long offset = 0;
    int  connected = 1;   /* состояние связи — чтобы не спамить в лог */
    while (!VTL_bot_g_stop) {
        char* body = VTL_bot_api_GetUpdates(token, offset, VTL_BOT_POLL_TIMEOUT_S);
        if (body) {
            if (!connected) {
                printf("[bot] связь с Telegram восстановлена.\n");
                fflush(stdout);
                connected = 1;
            }
            offset = VTL_bot_ProcessUpdates(body, &ctx, offset);
            free(body);
        } else {
            if (connected) {
                fprintf(stderr, "[bot] getUpdates: нет связи с Telegram "
                                "(сеть/TLS-сертификаты?). Повторяю...\n");
                connected = 0;
            }
            VTL_BOT_SLEEP(3); /* не крутим тугой цикл при постоянной ошибке */
        }
    }

    printf("\n[bot] остановлен.\n");
    return VTL_res_kOk;
}
