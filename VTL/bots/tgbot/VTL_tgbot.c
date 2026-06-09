#include <VTL/bots/tgbot/VTL_tgbot.h>
#include <VTL/bots/tgbot/VTL_tgbot_data.h>
#include <VTL/bots/tgbot/api/VTL_tgbot_api.h>
#include <VTL/bots/tgbot/session/VTL_tgbot_session.h>
#include <VTL/bots/tgbot/command/VTL_tgbot_command.h>

#include <parson/parson.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#  include <windows.h>
#  define VTL_tgbot_SLEEP(sec) Sleep((unsigned long)(sec) * 1000)
#else
#  include <unistd.h>
#  define VTL_tgbot_SLEEP(sec) sleep((unsigned int)(sec))
#endif

/* Флаг останова. */
static volatile sig_atomic_t VTL_tgbot_g_stop = 0;

static void VTL_tgbot_OnSignal(int sig)
{
    (void)sig;
    VTL_tgbot_g_stop = 1;
}

/* Отправляет ответ в чат через Telegram Bot API. */
static void VTL_tgbot_TelegramReply(void* sink_ud, const char* chat_id,
                                  const char* text)
{
    const char* token = (const char*)sink_ud;
    if (VTL_tgbot_api_SendMessage(token, chat_id, text) != VTL_res_kOk)
        fprintf(stderr, "[bot] sendMessage в чат %s не удался\n", chat_id);
}

/* Разбирает ответ getUpdates и возвращает новый offset. */
static long VTL_tgbot_ProcessUpdates(const char* body, VTL_tgbot_Context* ctx,
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
        VTL_tgbot_command_Handle(ctx, chat_id, text);
    }

    json_value_free(root);
    return offset;
}

VTL_AppResult VTL_tgbot_Run(const char* token, const VTL_tgbot_Handlers* handlers)
{
    if (!token || !*token) return VTL_res_kInvalidParamErr;

    VTL_tgbot_sessionTable sessions;
    VTL_tgbot_session_TableInit(&sessions);

    VTL_tgbot_Context ctx;
    ctx.sessions = &sessions;
    ctx.handlers = handlers;
    ctx.reply    = VTL_tgbot_TelegramReply;
    ctx.reply_ud = (void*)token;

    signal(SIGINT, VTL_tgbot_OnSignal);
#ifdef SIGTERM
    signal(SIGTERM, VTL_tgbot_OnSignal);
#endif

    printf("[bot] запущен. Ctrl+C для остановки.\n");
    fflush(stdout);

    char bot_name[64];
    if (VTL_tgbot_api_GetMe(token, bot_name, sizeof(bot_name))) {
        printf("[bot] подключён как @%s\n", bot_name);
        fflush(stdout);
    }

    /* Сброс webhook перед long-polling. */
    if (VTL_tgbot_api_DeleteWebhook(token)) {
        printf("[bot] webhook сброшен (long-polling готов).\n");
        fflush(stdout);
    }

    long offset = 0;
    int  connected = 1;
    while (!VTL_tgbot_g_stop) {
        char* body = VTL_tgbot_api_GetUpdates(token, offset, VTL_tgbot_POLL_TIMEOUT_S);
        if (body) {
            if (!connected) {
                printf("[bot] связь с Telegram восстановлена.\n");
                fflush(stdout);
                connected = 1;
            }
            offset = VTL_tgbot_ProcessUpdates(body, &ctx, offset);
            free(body);
        } else {
            if (connected) {
                fprintf(stderr, "[bot] getUpdates: нет связи с Telegram "
                                "(сеть/TLS-сертификаты?). Повторяю...\n");
                connected = 0;
            }
            VTL_tgbot_SLEEP(3);
        }
    }

    printf("\n[bot] остановлен.\n");
    return VTL_res_kOk;
}
