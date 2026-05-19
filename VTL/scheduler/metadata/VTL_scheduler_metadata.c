#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include <VTL/scheduler/metadata/VTL_scheduler_metadata.h>

#include <parson/parson.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


/* ================================================================== */
/* Вспомогательный макрос: безопасное копирование строки из JSON       */
/* ================================================================== */
#define COPY_STR(dst, src, size)                    \
    do {                                            \
        if (src) {                                  \
            strncpy((dst), (src), (size) - 1);      \
            (dst)[(size) - 1] = '\0';               \
        } else {                                    \
            (dst)[0] = '\0';                        \
        }                                           \
    } while (0)


/* ================================================================== */
/* Десериализация                                                       */
/* ================================================================== */

VTL_AppResult VTL_scheduler_meta_DeserializeTG(const char*           json_str,
                                                VTL_scheduler_MetaTG* out)
{
    if (!json_str || !out) return VTL_res_kInvalidParamErr;
    memset(out, 0, sizeof(*out));

    JSON_Value*  root_val = json_parse_string(json_str);
    if (!root_val) {
        fprintf(stderr, "[scheduler_meta] TG: failed to parse JSON: %.80s\n",
                json_str);
        return VTL_res_kErr;
    }

    JSON_Object* root = json_value_get_object(root_val);
    if (!root) { json_value_free(root_val); return VTL_res_kErr; }

    COPY_STR(out->chat_id,    json_object_get_string(root, "chat_id"),    sizeof(out->chat_id));
    COPY_STR(out->parse_mode, json_object_get_string(root, "parse_mode"), sizeof(out->parse_mode));

    json_value_free(root_val);

    if (out->chat_id[0] == '\0') {
        fprintf(stderr, "[scheduler_meta] TG: missing 'chat_id'\n");
        return VTL_res_kErr;
    }
    return VTL_res_kOk;
}

VTL_AppResult VTL_scheduler_meta_DeserializeReddit(const char*               json_str,
                                                    VTL_scheduler_MetaReddit* out)
{
    if (!json_str || !out) return VTL_res_kInvalidParamErr;
    memset(out, 0, sizeof(*out));

    JSON_Value*  root_val = json_parse_string(json_str);
    if (!root_val) {
        fprintf(stderr, "[scheduler_meta] Reddit: failed to parse JSON: %.80s\n",
                json_str);
        return VTL_res_kErr;
    }

    JSON_Object* root = json_value_get_object(root_val);
    if (!root) { json_value_free(root_val); return VTL_res_kErr; }

    COPY_STR(out->subreddit, json_object_get_string(root, "subreddit"), sizeof(out->subreddit));
    COPY_STR(out->title,     json_object_get_string(root, "title"),     sizeof(out->title));

    json_value_free(root_val);

    if (out->subreddit[0] == '\0') {
        fprintf(stderr, "[scheduler_meta] Reddit: missing 'subreddit'\n");
        return VTL_res_kErr;
    }
    return VTL_res_kOk;
}

VTL_AppResult VTL_scheduler_meta_DeserializeVK(const char*           json_str,
                                                VTL_scheduler_MetaVK* out)
{
    if (!json_str || !out) return VTL_res_kInvalidParamErr;
    memset(out, 0, sizeof(*out));

    JSON_Value*  root_val = json_parse_string(json_str);
    if (!root_val) {
        fprintf(stderr, "[scheduler_meta] VK: failed to parse JSON: %.80s\n",
                json_str);
        return VTL_res_kErr;
    }

    JSON_Object* root = json_value_get_object(root_val);
    if (!root) { json_value_free(root_val); return VTL_res_kErr; }

    /* peer_id хранится как number */
    out->peer_id = (long long)json_object_get_number(root, "peer_id");

    json_value_free(root_val);
    return VTL_res_kOk;
}


/* ================================================================== */
/* Сериализация                                                         */
/* ================================================================== */

char* VTL_scheduler_meta_SerializeTG(const VTL_scheduler_MetaTG* meta)
{
    if (!meta) return NULL;

    JSON_Value*  root_val = json_value_init_object();
    JSON_Object* root     = json_value_get_object(root_val);

    json_object_set_string(root, "chat_id",    meta->chat_id);
    json_object_set_string(root, "parse_mode", meta->parse_mode);

    char* serialized = json_serialize_to_string(root_val);
    json_value_free(root_val);

    if (!serialized) return NULL;

    /* parson аллоцирует через свой аллокатор; делаем копию через strdup
     * чтобы вызывающий мог делать free() стандартным способом.        */
    char* result = strdup(serialized);
    json_free_serialized_string(serialized);
    return result;
}

char* VTL_scheduler_meta_SerializeReddit(const VTL_scheduler_MetaReddit* meta)
{
    if (!meta) return NULL;

    JSON_Value*  root_val = json_value_init_object();
    JSON_Object* root     = json_value_get_object(root_val);

    json_object_set_string(root, "subreddit", meta->subreddit);
    json_object_set_string(root, "title",     meta->title);

    char* serialized = json_serialize_to_string(root_val);
    json_value_free(root_val);

    if (!serialized) return NULL;
    char* result = strdup(serialized);
    json_free_serialized_string(serialized);
    return result;
}

char* VTL_scheduler_meta_SerializeVK(const VTL_scheduler_MetaVK* meta)
{
    if (!meta) return NULL;

    JSON_Value*  root_val = json_value_init_object();
    JSON_Object* root     = json_value_get_object(root_val);

    json_object_set_number(root, "peer_id", (double)meta->peer_id);

    char* serialized = json_serialize_to_string(root_val);
    json_value_free(root_val);

    if (!serialized) return NULL;
    char* result = strdup(serialized);
    json_free_serialized_string(serialized);
    return result;
}
