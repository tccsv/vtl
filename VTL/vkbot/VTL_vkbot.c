#include <VTL/vkbot/VTL_vkbot.h>
#include <VTL/vkbot/VTL_vkbot_data.h>
#include <VTL/vkbot/api/VTL_vkbot_api.h>
#include <VTL/vkbot/store/VTL_vkbot_store.h>
#include <VTL/vkbot/command/VTL_vkbot_command.h>

#include <parson/parson.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#  include <windows.h>
#  define VTL_VKBOT_SLEEP(sec) Sleep((unsigned long)(sec) * 1000)
#else
#  include <unistd.h>
#  define VTL_VKBOT_SLEEP(sec) sleep((unsigned int)(sec))
#endif

/* Флаг останова, выставляется обработчиком сигнала. */
static volatile sig_atomic_t VTL_vkbot_g_stop = 0;

static void VTL_vkbot_OnSignal(int sig)
{
    (void)sig;
    VTL_vkbot_g_stop = 1;
}

/* Результат обработки пачки апдейтов. */
typedef enum {
    VTL_VKBOT_POLL_OK = 0,   /* ts обновлён, можно продолжать */
    VTL_VKBOT_POLL_REFETCH   /* ключ/сессия устарели — нужен новый long-poll сервер */
} VTL_vkbot_PollResult;

/* Разбирает ответ act=a_check: обновляет ts и обрабатывает события.
 * При failed:2/3 (ключ/сессия устарели) просит перезапросить сервер. */
static VTL_vkbot_PollResult VTL_vkbot_ProcessUpdates(const char* body,
                                                     VTL_vkbot_Store* store,
                                                     const char* token,
                                                     char* ts, size_t ts_cap)
{
    JSON_Value* root = json_parse_string(body);
    if (!root) return VTL_VKBOT_POLL_OK;

    JSON_Object* obj = json_value_get_object(root);
    if (!obj) { json_value_free(root); return VTL_VKBOT_POLL_OK; }

    /* failed: 1 — устарел ts (берём новый), 2/3 — нужен новый сервер. */
    if (json_object_has_value(obj, "failed")) {
        int failed = (int)json_object_get_number(obj, "failed");
        VTL_vkbot_PollResult res = VTL_VKBOT_POLL_OK;
        if (failed == 1) {
            double new_ts = json_object_get_number(obj, "ts");
            snprintf(ts, ts_cap, "%.0f", new_ts);
        } else {
            res = VTL_VKBOT_POLL_REFETCH;
        }
        json_value_free(root);
        return res;
    }

    /* Новый ts — строкой или числом, в зависимости от версии API. */
    const char* ts_s = json_object_get_string(obj, "ts");
    if (ts_s) snprintf(ts, ts_cap, "%s", ts_s);
    else      snprintf(ts, ts_cap, "%.0f", json_object_get_number(obj, "ts"));

    JSON_Array* updates = json_object_get_array(obj, "updates");
    size_t count = updates ? json_array_get_count(updates) : 0;

    for (size_t i = 0; i < count; ++i) {
        JSON_Object* upd = json_array_get_object(updates, i);
        const char* type = json_object_get_string(upd, "type");
        if (!type || strcmp(type, "message_new") != 0) continue;

        /* object.message.{peer_id,text} (формат API v5.103+). */
        const char* text = json_object_dotget_string(upd, "object.message.text");
        if (!text) continue;
        double peer_num = json_object_dotget_number(upd, "object.message.peer_id");
        if (peer_num == 0) continue;

        char peer_id[32];
        snprintf(peer_id, sizeof(peer_id), "%lld", (long long)peer_num);

        printf("[vkbot] <- peer %s: %s\n", peer_id, text);
        fflush(stdout);
        VTL_vkbot_command_Handle(store, token, peer_id, text);
    }

    json_value_free(root);
    return VTL_VKBOT_POLL_OK;
}

VTL_AppResult VTL_vkbot_Run(const char* token, const char* group_id,
                            const char* store_path)
{
    if (!token || !*token)       return VTL_res_kInvalidParamErr;
    if (!group_id || !*group_id) return VTL_res_kInvalidParamErr;
    if (!store_path) store_path = VTL_VKBOT_DEFAULT_STORE;

    VTL_vkbot_Store* store = VTL_vkbot_store_Open(store_path);
    if (!store) {
        fprintf(stderr, "[vkbot] не удалось открыть хранилище %s\n", store_path);
        return VTL_res_kFileOpenErr;
    }

    signal(SIGINT, VTL_vkbot_OnSignal);
#ifdef SIGTERM
    signal(SIGTERM, VTL_vkbot_OnSignal);
#endif

    printf("[vkbot] запущен, хранилище: %s. Ctrl+C для остановки.\n", store_path);
    fflush(stdout); /* в составе VTL stdout блочно-буферизован — форсим вывод */

    char server[512], key[256], ts[64];
    if (!VTL_vkbot_api_GetLongPollServer(token, group_id,
                                         server, sizeof(server),
                                         key, sizeof(key), ts, sizeof(ts))) {
        fprintf(stderr, "[vkbot] не удалось получить long-poll сервер "
                        "(токен сообщества/group_id/Long Poll API?).\n");
        VTL_vkbot_store_Close(store);
        return VTL_res_kErr;
    }
    printf("[vkbot] long-poll сервер получен, группа %s.\n", group_id);
    fflush(stdout);

    int connected = 1;   /* состояние связи — чтобы не спамить в лог */
    while (!VTL_vkbot_g_stop) {
        char* body = VTL_vkbot_api_GetUpdates(server, key, ts, VTL_VKBOT_POLL_WAIT_S);
        if (body) {
            if (!connected) {
                printf("[vkbot] связь с VK восстановлена.\n");
                fflush(stdout);
                connected = 1;
            }
            VTL_vkbot_PollResult res =
                VTL_vkbot_ProcessUpdates(body, store, token, ts, sizeof(ts));
            free(body);

            if (res == VTL_VKBOT_POLL_REFETCH) {
                /* Ключ/сессия устарели — перезапрашиваем сервер. */
                if (VTL_vkbot_api_GetLongPollServer(token, group_id,
                                                    server, sizeof(server),
                                                    key, sizeof(key),
                                                    ts, sizeof(ts))) {
                    printf("[vkbot] long-poll сессия обновлена.\n");
                    fflush(stdout);
                } else {
                    VTL_VKBOT_SLEEP(3);
                }
            }
        } else {
            if (connected) {
                fprintf(stderr, "[vkbot] a_check: нет связи с VK "
                                "(сеть/TLS-сертификаты?). Повторяю...\n");
                connected = 0;
            }
            VTL_VKBOT_SLEEP(3); /* не крутим тугой цикл при постоянной ошибке */
            /* Пробуем перезапросить сервер — вдруг устарела сессия. */
            VTL_vkbot_api_GetLongPollServer(token, group_id,
                                            server, sizeof(server),
                                            key, sizeof(key), ts, sizeof(ts));
        }
    }

    printf("\n[vkbot] остановлен.\n");
    VTL_vkbot_store_Close(store);
    return VTL_res_kOk;
}
