#include <VTL/vkbot/VTL_vk_bot.h>
#include <VTL/vkbot/VTL_vk_api.h>
#include <VTL/vkbot/VTL_vk_dialog.h>

#include <parson/parson.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#  include <windows.h>
#  define VTL_VK_NAP(s) Sleep((unsigned long)(s) * 1000)
#else
#  include <unistd.h>
#  define VTL_VK_NAP(s) sleep((unsigned int)(s))
#endif

static volatile sig_atomic_t vk_stop = 0;
static void vk_on_signal(int s) { (void)s; vk_stop = 1; }

/* Прод-канал ответа: emit_ud — токен сообщества. */
static void vk_emit_send(void* ud, long long peer_id, const char* text)
{
    const char* token = (const char*)ud;
    if (!VTL_vk_api_Reply(token, peer_id, text))
        fprintf(stderr, "[vk] не доставлено в peer %lld\n", peer_id);
}

/* Разбирает ответ a_check: прогоняет message_new через хаб, возвращает 1 если
 * надо переоткрыть long-poll (failed 2/3), иначе 0. Новый ts пишет в lp. */
static int vk_consume(const char* body, VTL_vk_Hub* hub, VTL_vk_LongPoll* lp)
{
    JSON_Value* root = json_parse_string(body);
    if (!root) return 0;
    JSON_Object* o = json_value_get_object(root);

    if (o && json_object_has_value(o, "failed")) {
        int code = (int)json_object_get_number(o, "failed");
        if (code == 1) {  /* устаревший ts — VK прислал новый */
            const char* ts = json_object_get_string(o, "ts");
            double tsn = json_object_get_number(o, "ts");
            if (ts) snprintf(lp->ts, sizeof(lp->ts), "%s", ts);
            else if (tsn > 0) snprintf(lp->ts, sizeof(lp->ts), "%.0f", tsn);
            json_value_free(root);
            return 0;
        }
        json_value_free(root);   /* 2/3 — ключ/сервер протухли */
        return 1;
    }

    const char* ts = o ? json_object_get_string(o, "ts") : NULL;
    if (ts) snprintf(lp->ts, sizeof(lp->ts), "%s", ts);

    JSON_Array* ups = o ? json_object_get_array(o, "updates") : NULL;
    size_t n = ups ? json_array_get_count(ups) : 0;
    for (size_t i = 0; i < n; ++i) {
        JSON_Object* up = json_array_get_object(ups, i);
        const char* type = up ? json_object_get_string(up, "type") : NULL;
        if (!type || strcmp(type, "message_new") != 0) continue;

        JSON_Object* msg = json_object_dotget_object(up, "object.message");
        if (!msg) continue;
        const char* txt = json_object_get_string(msg, "text");
        long long peer = (long long)json_object_get_number(msg, "peer_id");
        if (!txt || peer == 0) continue;

        printf("[vk] peer %lld: %s\n", peer, txt);
        fflush(stdout);
        VTL_vk_hub_Handle(hub, peer, txt);
    }

    json_value_free(root);
    return 0;
}

VTL_AppResult VTL_vk_bot_Serve(const char* token, const char* group_id,
                               const VTL_vk_Publishers* pub)
{
    if (!token || !*token || !group_id || !*group_id) return VTL_res_kInvalidParamErr;

    VTL_vk_Hub hub;
    VTL_vk_hub_Reset(&hub, pub, vk_emit_send, (void*)token);

    signal(SIGINT, vk_on_signal);
#ifdef SIGTERM
    signal(SIGTERM, vk_on_signal);
#endif

    VTL_vk_LongPoll lp;
    if (!VTL_vk_api_OpenLongPoll(token, group_id, &lp)) {
        fprintf(stderr, "[vk] не удалось открыть long-poll (токен/группа?).\n");
        return VTL_res_kErr;
    }
    printf("[vk] сообщество %s на связи, long-poll открыт. Ctrl+C — стоп.\n", group_id);
    fflush(stdout);

    while (!vk_stop) {
        char* body = VTL_vk_api_Check(&lp);
        if (!body) {                       /* сетевой сбой — пауза и повтор */
            VTL_VK_NAP(3);
            continue;
        }
        int reopen = vk_consume(body, &hub, &lp);
        free(body);
        if (reopen) {
            if (!VTL_vk_api_OpenLongPoll(token, group_id, &lp)) VTL_VK_NAP(3);
            else { printf("[vk] long-poll переоткрыт.\n"); fflush(stdout); }
        }
    }

    printf("\n[vk] остановлен.\n");
    return VTL_res_kOk;
}
