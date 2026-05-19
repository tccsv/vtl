#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif
#endif

#include <VTL/scheduler/dispatcher/VTL_scheduler_dispatcher.h>
#include <VTL/scheduler/metadata/VTL_scheduler_metadata.h>

/* Платформенные отправщики */
#include <VTL/content_platform/tg/VTL_content_platform_tg_net.h>
#include <VTL/content_platform/reddit/VTL_content_platform_reddit_net.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#ifdef _WIN32
#  include <windows.h>
#  define vtl_sleep_sec(s) Sleep((DWORD)((s) * 1000))
#else

#  include <unistd.h>

#  define vtl_sleep_sec(s) sleep((unsigned)(s))
#endif


/* ================================================================== */
/* Внутренние отправщики по типу соц-сети                              */
/* ================================================================== */

/* Мьютекс защищает связку setenv → SendNow от гонки между потоками.
 * TG-сервис читает TG_CHAT_ID из env — менять его и вызывать SendNow
 * нужно атомарно.                                                      */
static pthread_mutex_t s_tg_env_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ---- Telegram ---------------------------------------------------- */
static VTL_AppResult send_tg(const VTL_scheduler_Post *post) {
    VTL_scheduler_MetaTG meta;
    if (VTL_scheduler_meta_DeserializeTG(post->metadata, &meta) != VTL_res_kOk) {
        fprintf(stderr, "[dispatcher] TG post id=%lld: bad metadata\n", post->id);
        return VTL_res_kErr;
    }

    VTL_AppResult res;

    pthread_mutex_lock(&s_tg_env_mutex);

#ifndef _WIN32
    setenv("TG_CHAT_ID", meta.chat_id, 1);
#else
    SetEnvironmentVariableA("TG_CHAT_ID", meta.chat_id);
#endif

    if (post->content_type == VTL_content_kTxt) {
        /* Текст в post->content — пишем во временный файл и отправляем
         * через готовые методы TG-сервиса.                             */
        char tmp_path[256];
        snprintf(tmp_path, sizeof(tmp_path), "/tmp/vtl_sched_%lld.txt", post->id);
        FILE *f = fopen(tmp_path, "wb");
        if (!f) {
            pthread_mutex_unlock(&s_tg_env_mutex);
            return VTL_res_kFileOpenErr;
        }
        fwrite(post->content, 1, strlen(post->content), f);
        fclose(f);

        if (meta.parse_mode[0] != '\0') {
            res = VTL_content_platform_tg_marked_text_SendNow(tmp_path);
        } else {
            res = VTL_content_platform_tg_text_SendNow(tmp_path);
        }
        remove(tmp_path);

    } else {
        /* FILE_PATH: определяем тип по расширению */
        const char *path = post->content;
        const char *ext = strrchr(path, '.');

        if (ext && (strcmp(ext, ".mp3") == 0 ||
                    strcmp(ext, ".ogg") == 0 ||
                    strcmp(ext, ".flac") == 0)) {
            res = VTL_content_platform_tg_audio_SendNow(path);
        } else if (ext && (strcmp(ext, ".mp4") == 0 ||
                           strcmp(ext, ".mkv") == 0)) {
            res = VTL_content_platform_tg_video_SendNow(path);
        } else if (ext && (strcmp(ext, ".jpg") == 0 ||
                           strcmp(ext, ".jpeg") == 0 ||
                           strcmp(ext, ".png") == 0)) {
            res = VTL_content_platform_tg_photo_SendNow(path);
        } else {
            res = VTL_content_platform_tg_document_SendNow(path);
        }
    }

    pthread_mutex_unlock(&s_tg_env_mutex);
    return res;
}

/* ---- Reddit ------------------------------------------------------ */
static VTL_AppResult send_reddit(const VTL_scheduler_Post *post) {
    VTL_scheduler_MetaReddit meta;
    if (VTL_scheduler_meta_DeserializeReddit(post->metadata, &meta) != VTL_res_kOk) {
        fprintf(stderr, "[dispatcher] Reddit post id=%lld: bad metadata\n", post->id);
        return VTL_res_kErr;
    }

    if (post->content_type == VTL_content_kFilePath) {
        /* Фото / видео с MD-описанием рядом — ищем .md рядом с файлом */
        char md_path[512];
        snprintf(md_path, sizeof(md_path), "%s.md", post->content);
        return VTL_wrapped_reddit_upload_photo(meta.subreddit,
                                               post->content,
                                               md_path);
    } else {
        /* Текстовый пост: создаём временный файл */
        char tmp_path[256];
        snprintf(tmp_path, sizeof(tmp_path), "/tmp/vtl_sched_%lld.txt", post->id);
        FILE *f = fopen(tmp_path, "wb");
        if (!f) return VTL_res_kFileOpenErr;
        fwrite(post->content, 1, strlen(post->content), f);
        fclose(f);
        VTL_AppResult res = VTL_wrapped_reddit_send_text(meta.subreddit, tmp_path);
        remove(tmp_path);
        return res;
    }
}

/* ---- VK ---------------------------------------------------------- */
/* VK-драйвер в проекте пока не реализован — заглушка с логом.        */
static VTL_AppResult send_vk(const VTL_scheduler_Post *post) {
    VTL_scheduler_MetaVK meta;
    if (VTL_scheduler_meta_DeserializeVK(post->metadata, &meta) != VTL_res_kOk) {
        fprintf(stderr, "[dispatcher] VK post id=%lld: bad metadata\n", post->id);
        return VTL_res_kErr;
    }

    fprintf(stderr, "[dispatcher] VK send stub: peer_id=%lld content=%.60s\n",
            meta.peer_id,
            post->content ? post->content : "(file)");

    /* TODO: подключить VK Bot API когда будет готов драйвер */
    return VTL_res_kOk;
}


/* ================================================================== */
/* Публичный dispatcher: одна запись                                   */
/* ================================================================== */

VTL_AppResult VTL_scheduler_dispatcher_Send(VTL_scheduler_Repo *repo,
                                            const VTL_scheduler_Post *post) {
    if (!post) return VTL_res_kInvalidParamErr;

    fprintf(stdout, "[dispatcher] sending post id=%lld sn=%s\n",
            post->id, post->sn_type == VTL_sn_kTG ? "TG" :
                      post->sn_type == VTL_sn_kReddit ? "REDDIT" : "VK");

    VTL_AppResult res;
    switch (post->sn_type) {
        case VTL_sn_kTG:
            res = send_tg(post);
            break;
        case VTL_sn_kReddit:
            res = send_reddit(post);
            break;
        case VTL_sn_kVK:
            res = send_vk(post);
            break;
        default:
            fprintf(stderr, "[dispatcher] post id=%lld: unknown sn_type\n",
                    post->id);
            return VTL_res_kErr;
    }

    if (res == VTL_res_kOk) {
        if (repo) VTL_scheduler_repo_MarkExecuted(repo, post->id);
        fprintf(stdout, "[dispatcher] post id=%lld sent OK\n", post->id);
    } else {
        fprintf(stderr, "[dispatcher] post id=%lld send FAILED (code %d)\n",
                post->id, (int) res);
    }

    return res;
}


/* ================================================================== */
/* Параллельная отправка пакета                                        */
/* ================================================================== */

/* Контекст потока — одна запись + указатель на репозиторий            */
typedef struct _VTL_sched_WorkerCtx {
    VTL_scheduler_Repo *repo;
    VTL_scheduler_Post post;   /* копия, поток владеет строками */
} VTL_sched_WorkerCtx;

static void *worker_thread(void *arg) {
    VTL_sched_WorkerCtx *ctx = (VTL_sched_WorkerCtx *) arg;
    VTL_scheduler_dispatcher_Send(ctx->repo, &ctx->post);
    VTL_scheduler_post_Free(&ctx->post);
    free(ctx);
    return NULL;
}

/* Глубокое копирование строк записи (каждый поток владеет своими)    */
static VTL_scheduler_Post post_deep_copy(const VTL_scheduler_Post *src) {
    VTL_scheduler_Post dst = *src;
    dst.created_user = src->created_user ? strdup(src->created_user) : NULL;
    dst.content = src->content ? strdup(src->content) : NULL;
    dst.metadata = src->metadata ? strdup(src->metadata) : NULL;
    return dst;
}

/* Запустить N потоков (по одному на запись), подождать всех.          */
static void dispatch_batch_parallel(VTL_scheduler_Repo *repo,
                                    const VTL_scheduler_PostList *list) {
    if (!list || list->length == 0) return;

    pthread_t *threads = (pthread_t *) malloc(list->length * sizeof(pthread_t));
    if (!threads) return;

    size_t spawned = 0;
    for (size_t i = 0; i < list->length; ++i) {
        VTL_sched_WorkerCtx *ctx =
                (VTL_sched_WorkerCtx *) malloc(sizeof(VTL_sched_WorkerCtx));
        if (!ctx) break;

        ctx->repo = repo;
        ctx->post = post_deep_copy(&list->items[i]);

        if (pthread_create(&threads[spawned], NULL, worker_thread, ctx) != 0) {
            /* Не удалось создать поток — отправляем синхронно        */
            fprintf(stderr, "[dispatcher] pthread_create failed for id=%lld,"
                            " falling back to sync\n", list->items[i].id);
            VTL_scheduler_dispatcher_Send(repo, &list->items[i]);
            VTL_scheduler_post_Free(&ctx->post);
            free(ctx);
        } else {
            ++spawned;
        }
    }

    for (size_t i = 0; i < spawned; ++i)
        pthread_join(threads[i], NULL);

    free(threads);
}


/* ================================================================== */
/* Главный цикл                                                        */
/* ================================================================== */

void VTL_scheduler_dispatcher_Run(VTL_scheduler_Repo *repo,
                                  int poll_interval_sec,
                                  int lookahead_sec,
                                  volatile int *stop_flag) {
    if (!repo || !stop_flag) return;

    fprintf(stdout, "[dispatcher] started: poll=%ds lookahead=%ds\n",
            poll_interval_sec, lookahead_sec);

    while (!(*stop_flag)) {
        VTL_scheduler_PostList list;
        memset(&list, 0, sizeof(list));

        VTL_AppResult res = VTL_scheduler_repo_GetPending(repo,
                                                          lookahead_sec,
                                                          &list);
        if (res == VTL_res_kOk && list.length > 0) {
            fprintf(stdout, "[dispatcher] found %zu pending post(s)\n",
                    list.length);
            dispatch_batch_parallel(repo, &list);
        }

        VTL_scheduler_postlist_Free(&list);

        /* Ждём следующего опроса (секундными шагами для быстрой реакции
         * на stop_flag)                                               */
        for (int s = 0; s < poll_interval_sec && !(*stop_flag); ++s)
            vtl_sleep_sec(1);
    }

    fprintf(stdout, "[dispatcher] stopped\n");
}