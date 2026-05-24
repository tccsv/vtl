#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif
#endif

#include <VTL/VTL.h>
#include <VTL/publication/text/asciidoc/VTL_publication_text_op_asciidoc.h>
#include <VTL/publication/text/asciidoc/VTL_publication_text_op_asciidoc_compat.h>
#include <VTL/scheduler/db/VTL_scheduler_db_repo.h>
#include <VTL/scheduler/dispatcher/VTL_scheduler_dispatcher.h>
#include <VTL/scheduler/metadata/VTL_scheduler_metadata.h>
#include <VTL/content_platform/vimeo/VTL_content_platform_vimeo_net.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef _WIN32

#include <unistd.h>

#else
#include <windows.h>
#include <io.h>
#define access _access
#define F_OK 0
#endif


static long detect_cpu_cores(void) {
#ifdef _WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (long)si.dwNumberOfProcessors;
#else
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return (n > 0) ? n : 1;
#endif
}


/* -----------------------------------------------------------------------
 * Вспомогательная функция: заполняет VTL_db_Credentals из env-переменных.
 * Переменные: DB_HOST, DB_PORT, DB_USER, DB_PASSWORD, DB_NAME.
 * Если переменная не задана — используется значение по умолчанию.
 * ----------------------------------------------------------------------- */
static void scheduler_db_creds_from_env(VTL_db_Credentals *creds) {
    const char *host = getenv("DB_HOST");
    const char *port = getenv("DB_PORT");
    const char *user = getenv("DB_USER");
    const char *password = getenv("DB_PASSWORD");
    const char *dbname = getenv("DB_NAME");

    creds->host = (char *) (host ? host : "127.0.0.1");
    creds->port = (char *) (port ? port : "5432");
    creds->user = (char *) (user ? user : "postgres");
    creds->password = (char *) (password ? password : "123");
    creds->dbname = (char *) (dbname ? dbname : "postgres");
}


static int g_tests_run = 0;
static int g_tests_failed = 0;

#define TEST_ASSERT(cond, msg)                                      \
    do {                                                            \
        ++g_tests_run;                                              \
        if (!(cond)) {                                              \
            ++g_tests_failed;                                       \
            printf("  [FAIL] %s\n", (msg));                        \
        } else {                                                    \
            printf("  [OK]   %s\n", (msg));                        \
        }                                                           \
        fflush(stdout);                                             \
    } while (0)

#define TEST_SECTION(name) \
    do { printf("\n--- %s ---\n", (name)); fflush(stdout); } while (0)

static int test_repo_setup(VTL_scheduler_Repo *repo) {
    memset(repo, 0, sizeof(*repo));

    VTL_db_Credentals creds;
    scheduler_db_creds_from_env(&creds);

    if (VTL_scheduler_repo_Open(repo, &creds) != VTL_res_kOk) {
        printf("  БД недоступна — тесты scheduler_repo пропускаются.\n");
        fflush(stdout);
        return 0;
    }
    if (VTL_scheduler_repo_EnsureTable(repo) != VTL_res_kOk) {
        printf("  EnsureTable завершилась с ошибкой — тесты пропускаются.\n");
        fflush(stdout);
        VTL_scheduler_repo_Close(repo);
        return 0;
    }
    return 1;
}

static void test_repo_open_close(void) {
    TEST_SECTION("Open / Close");

    /* NULL-защита — не требует БД */
    TEST_ASSERT(VTL_scheduler_repo_Open(NULL, NULL) == VTL_res_kInvalidParamErr,
                "Open(NULL, NULL) → kInvalidParamErr");

    VTL_scheduler_Repo repo;
    memset(&repo, 0, sizeof(repo));
    VTL_db_Credentals creds;
    scheduler_db_creds_from_env(&creds);

    TEST_ASSERT(VTL_scheduler_repo_Open(NULL, &creds) == VTL_res_kInvalidParamErr,
                "Open(NULL, creds) → kInvalidParamErr");
    TEST_ASSERT(VTL_scheduler_repo_Open(&repo, NULL) == VTL_res_kInvalidParamErr,
                "Open(repo, NULL) → kInvalidParamErr");

    /* Close на нулевом conn — не должен падать */
    VTL_scheduler_repo_Close(&repo);
    TEST_ASSERT(repo.conn == NULL, "Close на repo с conn=NULL не падает");

    /* Подключение к БД — только если она доступна */
    VTL_AppResult res = VTL_scheduler_repo_Open(&repo, &creds);
    if (res != VTL_res_kOk) {
        printf("  БД недоступна — тест подключения пропускается.\n");
        fflush(stdout);
        return;
    }

    TEST_ASSERT(repo.conn != NULL, "repo.conn != NULL после успешного Open");
    VTL_scheduler_repo_Close(&repo);
    TEST_ASSERT(repo.conn == NULL, "repo.conn == NULL после Close");
    VTL_scheduler_repo_Close(&repo);
    TEST_ASSERT(1, "Повторный Close не вызывает краша");
}

static void test_repo_open_null_params(void) {
    TEST_SECTION("Open — NULL-аргументы");

    VTL_db_Credentals creds;
    scheduler_db_creds_from_env(&creds);

    VTL_scheduler_Repo repo;
    memset(&repo, 0, sizeof(repo));

    TEST_ASSERT(VTL_scheduler_repo_Open(NULL, &creds) == VTL_res_kInvalidParamErr,
                "Open(NULL, creds) → kInvalidParamErr");
    TEST_ASSERT(VTL_scheduler_repo_Open(&repo, NULL) == VTL_res_kInvalidParamErr,
                "Open(repo, NULL) → kInvalidParamErr");
}

static void test_repo_ensure_table(void) {
    TEST_SECTION("EnsureTable");

    VTL_scheduler_Repo repo;
    if (!test_repo_setup(&repo)) return;

    VTL_AppResult res = VTL_scheduler_repo_EnsureTable(&repo);
    TEST_ASSERT(res == VTL_res_kOk, "Второй вызов EnsureTable идемпотентен");

    TEST_ASSERT(VTL_scheduler_repo_EnsureTable(NULL) == VTL_res_kInvalidParamErr,
                "EnsureTable(NULL) → kInvalidParamErr");

    VTL_scheduler_repo_Close(&repo);
}

static void test_repo_insert(void) {
    TEST_SECTION("Insert");

    VTL_scheduler_Repo repo;
    if (!test_repo_setup(&repo)) return;

    VTL_scheduler_Post post;
    memset(&post, 0, sizeof(post));
    post.send_date_time = time(NULL) + 120;
    post.created_user = "test_insert";
    post.content_type = VTL_content_kTxt;
    post.content = "Тест Insert";
    post.sn_type = VTL_sn_kTG;
    post.metadata = "{\"chat_id\":\"@test\",\"parse_mode\":\"MarkdownV2\"}";
    post.executed = false;

    VTL_AppResult res = VTL_scheduler_repo_Insert(&repo, &post);
    TEST_ASSERT(res == VTL_res_kOk, "Insert возвращает kOk");
    TEST_ASSERT(post.id > 0, "Insert заполняет post.id > 0");

    TEST_ASSERT(VTL_scheduler_repo_Insert(NULL, &post) == VTL_res_kInvalidParamErr,
                "Insert(NULL, post) → kInvalidParamErr");
    TEST_ASSERT(VTL_scheduler_repo_Insert(&repo, NULL) == VTL_res_kInvalidParamErr,
                "Insert(repo, NULL) → kInvalidParamErr");

    VTL_scheduler_repo_Delete(&repo, post.id);
    VTL_scheduler_repo_Close(&repo);
}

static void test_repo_get_by_id(void) {
    TEST_SECTION("GetById");

    VTL_scheduler_Repo repo;
    if (!test_repo_setup(&repo)) return;

    VTL_scheduler_Post post;
    memset(&post, 0, sizeof(post));
    post.send_date_time = time(NULL) + 60;
    post.created_user = "test_getbyid";
    post.content_type = VTL_content_kTxt;
    post.content = "Контент для GetById";
    post.sn_type = VTL_sn_kReddit;
    post.metadata = "{\"subreddit\":\"test\",\"title\":\"vtl\"}";
    post.executed = false;

    VTL_scheduler_repo_Insert(&repo, &post);

    VTL_scheduler_Post fetched;
    memset(&fetched, 0, sizeof(fetched));
    VTL_AppResult res = VTL_scheduler_repo_GetById(&repo, post.id, &fetched);

    TEST_ASSERT(res == VTL_res_kOk, "GetById находит существующую запись");
    TEST_ASSERT(fetched.id == post.id, "Возвращённый id совпадает с вставленным");
    TEST_ASSERT(fetched.content != NULL && strcmp(fetched.content, post.content) == 0,
                "Возвращённый content совпадает с вставленным");
    TEST_ASSERT(fetched.sn_type == VTL_sn_kReddit, "Возвращённый sn_type == kReddit");
    TEST_ASSERT(fetched.executed == false, "Возвращённый executed == false");
    VTL_scheduler_post_Free(&fetched);

    TEST_ASSERT(VTL_scheduler_repo_GetById(&repo, -999999LL, &fetched) != VTL_res_kOk,
                "GetById с несуществующим id возвращает ошибку");

    TEST_ASSERT(VTL_scheduler_repo_GetById(NULL, post.id, &fetched) == VTL_res_kInvalidParamErr,
                "GetById(NULL, id, out) → kInvalidParamErr");
    TEST_ASSERT(VTL_scheduler_repo_GetById(&repo, post.id, NULL) == VTL_res_kInvalidParamErr,
                "GetById(repo, id, NULL) → kInvalidParamErr");

    VTL_scheduler_repo_Delete(&repo, post.id);
    VTL_scheduler_repo_Close(&repo);
}

static void test_repo_update(void) {
    TEST_SECTION("Update");

    VTL_scheduler_Repo repo;
    if (!test_repo_setup(&repo)) return;

    VTL_scheduler_Post post;
    memset(&post, 0, sizeof(post));
    post.send_date_time = time(NULL) + 60;
    post.created_user = "test_update";
    post.content_type = VTL_content_kTxt;
    post.content = "Оригинальный контент";
    post.sn_type = VTL_sn_kTG;
    post.metadata = "{\"chat_id\":\"@test\"}";
    post.executed = false;

    VTL_scheduler_repo_Insert(&repo, &post);

    post.content = "Обновлённый контент";
    post.sn_type = VTL_sn_kVK;
    post.send_date_time += 300;

    VTL_AppResult res = VTL_scheduler_repo_Update(&repo, &post);
    TEST_ASSERT(res == VTL_res_kOk, "Update возвращает kOk");

    VTL_scheduler_Post fetched;
    memset(&fetched, 0, sizeof(fetched));
    VTL_scheduler_repo_GetById(&repo, post.id, &fetched);
    TEST_ASSERT(strcmp(fetched.content, "Обновлённый контент") == 0,
                "После Update content изменился");
    TEST_ASSERT(fetched.sn_type == VTL_sn_kVK, "После Update sn_type изменился на kVK");
    VTL_scheduler_post_Free(&fetched);

    TEST_ASSERT(VTL_scheduler_repo_Update(NULL, &post) == VTL_res_kInvalidParamErr,
                "Update(NULL, post) → kInvalidParamErr");
    TEST_ASSERT(VTL_scheduler_repo_Update(&repo, NULL) == VTL_res_kInvalidParamErr,
                "Update(repo, NULL) → kInvalidParamErr");

    VTL_scheduler_repo_Delete(&repo, post.id);
    VTL_scheduler_repo_Close(&repo);
}

static void test_repo_mark_executed(void) {
    TEST_SECTION("MarkExecuted");

    VTL_scheduler_Repo repo;
    if (!test_repo_setup(&repo)) return;

    VTL_scheduler_Post post;
    memset(&post, 0, sizeof(post));
    post.send_date_time = time(NULL) + 60;
    post.created_user = "test_mark";
    post.content_type = VTL_content_kTxt;
    post.content = "Пост для MarkExecuted";
    post.sn_type = VTL_sn_kTG;
    post.metadata = "{\"chat_id\":\"@test\"}";
    post.executed = false;

    VTL_scheduler_repo_Insert(&repo, &post);

    VTL_AppResult res = VTL_scheduler_repo_MarkExecuted(&repo, post.id);
    TEST_ASSERT(res == VTL_res_kOk, "MarkExecuted возвращает kOk");

    VTL_scheduler_Post fetched;
    memset(&fetched, 0, sizeof(fetched));
    VTL_scheduler_repo_GetById(&repo, post.id, &fetched);
    TEST_ASSERT(fetched.executed == true,
                "После MarkExecuted флаг executed == true в БД");
    VTL_scheduler_post_Free(&fetched);

    TEST_ASSERT(VTL_scheduler_repo_MarkExecuted(NULL, post.id) == VTL_res_kInvalidParamErr,
                "MarkExecuted(NULL, id) → kInvalidParamErr");

    VTL_scheduler_repo_Delete(&repo, post.id);
    VTL_scheduler_repo_Close(&repo);
}

static void test_repo_delete(void) {
    TEST_SECTION("Delete");

    VTL_scheduler_Repo repo;
    if (!test_repo_setup(&repo)) return;

    VTL_scheduler_Post post;
    memset(&post, 0, sizeof(post));
    post.send_date_time = time(NULL) + 60;
    post.created_user = "test_delete";
    post.content_type = VTL_content_kTxt;
    post.content = "Пост для Delete";
    post.sn_type = VTL_sn_kTG;
    post.metadata = "{\"chat_id\":\"@test\"}";
    post.executed = false;

    VTL_scheduler_repo_Insert(&repo, &post);
    long long saved_id = post.id;

    VTL_AppResult res = VTL_scheduler_repo_Delete(&repo, saved_id);
    TEST_ASSERT(res == VTL_res_kOk, "Delete возвращает kOk");

    VTL_scheduler_Post fetched;
    memset(&fetched, 0, sizeof(fetched));
    TEST_ASSERT(VTL_scheduler_repo_GetById(&repo, saved_id, &fetched) != VTL_res_kOk,
                "После Delete GetById не находит запись");

    TEST_ASSERT(VTL_scheduler_repo_Delete(NULL, saved_id) == VTL_res_kInvalidParamErr,
                "Delete(NULL, id) → kInvalidParamErr");

    VTL_scheduler_repo_Close(&repo);
}

static void test_repo_get_pending(void) {
    TEST_SECTION("GetPending");

    VTL_scheduler_Repo repo;
    if (!test_repo_setup(&repo)) return;

    time_t now = time(NULL);

    VTL_scheduler_Post post_in;
    memset(&post_in, 0, sizeof(post_in));
    post_in.send_date_time = now + 60;
    post_in.created_user = "test_pending_in";
    post_in.content_type = VTL_content_kTxt;
    post_in.content = "Пост внутри окна";
    post_in.sn_type = VTL_sn_kTG;
    post_in.metadata = "{\"chat_id\":\"@test\"}";
    post_in.executed = false;
    VTL_scheduler_repo_Insert(&repo, &post_in);

    VTL_scheduler_Post post_out;
    memset(&post_out, 0, sizeof(post_out));
    post_out.send_date_time = now + 300;
    post_out.created_user = "test_pending_out";
    post_out.content_type = VTL_content_kTxt;
    post_out.content = "Пост за пределами окна";
    post_out.sn_type = VTL_sn_kTG;
    post_out.metadata = "{\"chat_id\":\"@test\"}";
    post_out.executed = false;
    VTL_scheduler_repo_Insert(&repo, &post_out);

    VTL_scheduler_Post post_done;
    memset(&post_done, 0, sizeof(post_done));
    post_done.send_date_time = now + 30;
    post_done.created_user = "test_pending_done";
    post_done.content_type = VTL_content_kTxt;
    post_done.content = "Исполненный пост";
    post_done.sn_type = VTL_sn_kTG;
    post_done.metadata = "{\"chat_id\":\"@test\"}";
    post_done.executed = false;
    VTL_scheduler_repo_Insert(&repo, &post_done);
    VTL_scheduler_repo_MarkExecuted(&repo, post_done.id);

    VTL_scheduler_PostList list;
    memset(&list, 0, sizeof(list));
    VTL_AppResult res = VTL_scheduler_repo_GetPending(&repo, 120, &list);
    TEST_ASSERT(res == VTL_res_kOk, "GetPending возвращает kOk");

    int found_in = 0, found_out = 0, found_done = 0;
    for (size_t i = 0; i < list.length; ++i) {
        if (list.items[i].id == post_in.id) found_in = 1;
        if (list.items[i].id == post_out.id) found_out = 1;
        if (list.items[i].id == post_done.id) found_done = 1;
    }
    TEST_ASSERT(found_in == 1, "GetPending возвращает пост внутри окна");
    TEST_ASSERT(found_out == 0, "GetPending не возвращает пост за пределами окна");
    TEST_ASSERT(found_done == 0, "GetPending не возвращает исполненный пост");
    VTL_scheduler_postlist_Free(&list);

    TEST_ASSERT(VTL_scheduler_repo_GetPending(NULL, 120, &list) == VTL_res_kInvalidParamErr,
                "GetPending(NULL, ...) → kInvalidParamErr");
    TEST_ASSERT(VTL_scheduler_repo_GetPending(&repo, 120, NULL) == VTL_res_kInvalidParamErr,
                "GetPending(..., NULL) → kInvalidParamErr");

    VTL_scheduler_repo_Delete(&repo, post_in.id);
    VTL_scheduler_repo_Delete(&repo, post_out.id);
    VTL_scheduler_repo_Delete(&repo, post_done.id);
    VTL_scheduler_repo_Close(&repo);
}

static void test_repo_insert_all_sn_types(void) {
    TEST_SECTION("Insert — все sn_type");

    VTL_scheduler_Repo repo;
    if (!test_repo_setup(&repo)) return;

    struct {
        VTL_scheduler_SnType sn;
        const char *label;
    } cases[] = {
            {VTL_sn_kTG,     "kTG"},
            {VTL_sn_kReddit, "kReddit"},
            {VTL_sn_kVK,     "kVK"},
    };
    size_t n = sizeof(cases) / sizeof(cases[0]);

    for (size_t i = 0; i < n; ++i) {
        VTL_scheduler_Post post;
        memset(&post, 0, sizeof(post));
        post.send_date_time = time(NULL) + 60;
        post.created_user = "test_sn_types";
        post.content_type = VTL_content_kTxt;
        post.content = "Контент sn-type теста";
        post.sn_type = cases[i].sn;
        post.metadata = "{}";
        post.executed = false;

        VTL_scheduler_repo_Insert(&repo, &post);

        VTL_scheduler_Post fetched;
        memset(&fetched, 0, sizeof(fetched));
        VTL_scheduler_repo_GetById(&repo, post.id, &fetched);

        char msg[64];
        snprintf(msg, sizeof(msg), "sn_type %s сохраняется и читается корректно", cases[i].label);
        TEST_ASSERT(fetched.sn_type == cases[i].sn, msg);
        VTL_scheduler_post_Free(&fetched);

        VTL_scheduler_repo_Delete(&repo, post.id);
    }

    VTL_scheduler_repo_Close(&repo);
}

static void test_repo_insert_content_types(void) {
    TEST_SECTION("Insert — content_type");

    VTL_scheduler_Repo repo;
    if (!test_repo_setup(&repo)) return;

    struct {
        VTL_scheduler_ContentType ct;
        const char *label;
    } cases[] = {
            {VTL_content_kTxt,      "kTxt"},
            {VTL_content_kFilePath, "kFilePath"},
    };
    size_t n = sizeof(cases) / sizeof(cases[0]);

    for (size_t i = 0; i < n; ++i) {
        VTL_scheduler_Post post;
        memset(&post, 0, sizeof(post));
        post.send_date_time = time(NULL) + 60;
        post.created_user = "test_ct";
        post.content_type = cases[i].ct;
        post.content = "/tmp/file.mp3";
        post.sn_type = VTL_sn_kTG;
        post.metadata = "{}";
        post.executed = false;

        VTL_scheduler_repo_Insert(&repo, &post);

        VTL_scheduler_Post fetched;
        memset(&fetched, 0, sizeof(fetched));
        VTL_scheduler_repo_GetById(&repo, post.id, &fetched);

        char msg[64];
        snprintf(msg, sizeof(msg), "content_type %s сохраняется и читается корректно", cases[i].label);
        TEST_ASSERT(fetched.content_type == cases[i].ct, msg);
        VTL_scheduler_post_Free(&fetched);

        VTL_scheduler_repo_Delete(&repo, post.id);
    }

    VTL_scheduler_repo_Close(&repo);
}

#define TG_TEST_BOT_TOKEN "8931801972:AAGcnJEW_WLBY2rUHwCc1-Z_msXyjH1-3b8"
#define TG_TEST_CHAT_ID   "-1003948115596"

static void tg_sched_setup_env(void) {
#ifndef _WIN32
    setenv("TG_BOT_TOKEN", TG_TEST_BOT_TOKEN, 1);
    setenv("TG_CHAT_ID", TG_TEST_CHAT_ID, 1);
#else
    SetEnvironmentVariableA("TG_BOT_TOKEN", TG_TEST_BOT_TOKEN);
    SetEnvironmentVariableA("TG_CHAT_ID",   TG_TEST_CHAT_ID);
#endif
}

static char *tg_sched_make_post(VTL_scheduler_Post *post,
                                const char *content,
                                const char *parse_mode) {
    VTL_scheduler_MetaTG meta;
    memset(&meta, 0, sizeof(meta));
    strncpy(meta.chat_id, TG_TEST_CHAT_ID, sizeof(meta.chat_id) - 1);
    strncpy(meta.parse_mode, parse_mode ? parse_mode : "", sizeof(meta.parse_mode) - 1);

    char *metadata = VTL_scheduler_meta_SerializeTG(&meta);

    memset(post, 0, sizeof(*post));
    post->send_date_time = time(NULL);
    post->created_user = "test_tg_sched";
    post->content_type = VTL_content_kTxt;
    post->content = (char *) content;
    post->sn_type = VTL_sn_kTG;
    post->metadata = metadata;
    post->executed = false;

    return metadata;
}


static void test_tg_sched_send_plain(void) {
    TEST_SECTION("dispatcher_Send — TG plain text");

    VTL_scheduler_Repo repo;
    if (!test_repo_setup(&repo)) return;

    VTL_scheduler_Post post;
    char *metadata = tg_sched_make_post(&post, "VTL scheduler test: plain text", "");
    post.metadata = metadata;

    VTL_AppResult ins = VTL_scheduler_repo_Insert(&repo, &post);
    TEST_ASSERT(ins == VTL_res_kOk, "Insert plain-text поста для TG возвращает kOk");

    VTL_AppResult send = VTL_scheduler_dispatcher_Send(&repo, &post);
    TEST_ASSERT(send == VTL_res_kOk, "dispatcher_Send plain-text → kOk");

    VTL_scheduler_Post fetched;
    memset(&fetched, 0, sizeof(fetched));
    VTL_scheduler_repo_GetById(&repo, post.id, &fetched);
    TEST_ASSERT(fetched.executed == true,
                "После dispatcher_Send запись помечена executed = true");
    VTL_scheduler_post_Free(&fetched);

    VTL_scheduler_repo_Delete(&repo, post.id);
    free(metadata);
    VTL_scheduler_repo_Close(&repo);
}


static void test_tg_sched_send_markdownv2(void) {
    TEST_SECTION("dispatcher_Send — TG MarkdownV2");

    VTL_scheduler_Repo repo;
    if (!test_repo_setup(&repo)) return;

    VTL_scheduler_Post post;
    char *metadata = tg_sched_make_post(
            &post,
            "*жирный* _курсив_ `код` VTL scheduler MarkdownV2 test",
            "MarkdownV2");
    post.metadata = metadata;

    VTL_scheduler_repo_Insert(&repo, &post);

    VTL_AppResult send = VTL_scheduler_dispatcher_Send(&repo, &post);
    TEST_ASSERT(send == VTL_res_kOk, "dispatcher_Send MarkdownV2 → kOk");

    VTL_scheduler_Post fetched;
    memset(&fetched, 0, sizeof(fetched));
    VTL_scheduler_repo_GetById(&repo, post.id, &fetched);
    TEST_ASSERT(fetched.executed == true,
                "После dispatcher_Send MarkdownV2 запись помечена executed = true");
    VTL_scheduler_post_Free(&fetched);

    VTL_scheduler_repo_Delete(&repo, post.id);
    free(metadata);
    VTL_scheduler_repo_Close(&repo);
}


static void test_tg_sched_send_unicode(void) {
    TEST_SECTION("dispatcher_Send — TG UTF-8 / кириллица / эмодзи");

    VTL_scheduler_Repo repo;
    if (!test_repo_setup(&repo)) return;

    VTL_scheduler_Post post;
    char *metadata = tg_sched_make_post(
            &post,
            "VTL тест: кириллица и эмодзи \xF0\x9F\x9A\x80",
            "");
    post.metadata = metadata;

    VTL_scheduler_repo_Insert(&repo, &post);

    VTL_AppResult send = VTL_scheduler_dispatcher_Send(&repo, &post);
    TEST_ASSERT(send == VTL_res_kOk, "dispatcher_Send с UTF-8 контентом → kOk");

    VTL_scheduler_Post fetched;
    memset(&fetched, 0, sizeof(fetched));
    VTL_scheduler_repo_GetById(&repo, post.id, &fetched);
    TEST_ASSERT(fetched.executed == true,
                "После dispatcher_Send UTF-8 запись помечена executed = true");
    VTL_scheduler_post_Free(&fetched);

    VTL_scheduler_repo_Delete(&repo, post.id);
    free(metadata);
    VTL_scheduler_repo_Close(&repo);
}


static void test_tg_sched_send_long_text(void) {
    TEST_SECTION("dispatcher_Send — TG длинный текст (~3000 символов)");

    VTL_scheduler_Repo repo;
    if (!test_repo_setup(&repo)) return;

    static char long_buf[3001];
    const char *pattern = "VTL long-message test. ";
    size_t plen = strlen(pattern), pos = 0;
    while (pos + plen < sizeof(long_buf) - 1) {
        memcpy(long_buf + pos, pattern, plen);
        pos += plen;
    }
    long_buf[pos] = '\0';

    VTL_scheduler_Post post;
    char *metadata = tg_sched_make_post(&post, long_buf, "");
    post.metadata = metadata;

    VTL_scheduler_repo_Insert(&repo, &post);

    VTL_AppResult send = VTL_scheduler_dispatcher_Send(&repo, &post);
    TEST_ASSERT(send == VTL_res_kOk,
                "dispatcher_Send длинного текста (~3000 символов) → kOk");

    VTL_scheduler_Post fetched;
    memset(&fetched, 0, sizeof(fetched));
    VTL_scheduler_repo_GetById(&repo, post.id, &fetched);
    TEST_ASSERT(fetched.executed == true,
                "После dispatcher_Send длинного текста запись помечена executed = true");
    VTL_scheduler_post_Free(&fetched);

    VTL_scheduler_repo_Delete(&repo, post.id);
    free(metadata);
    VTL_scheduler_repo_Close(&repo);
}


static void test_tg_sched_send_bad_metadata(void) {
    TEST_SECTION("dispatcher_Send — TG невалидный metadata JSON");

    VTL_scheduler_Repo repo;
    if (!test_repo_setup(&repo)) return;

    VTL_scheduler_Post post;
    memset(&post, 0, sizeof(post));
    post.send_date_time = time(NULL);
    post.created_user = "test_tg_bad_meta";
    post.content_type = VTL_content_kTxt;
    post.content = "Текст с плохим metadata";
    post.sn_type = VTL_sn_kTG;
    post.metadata = "{ not valid json at all !!!";
    post.executed = false;

    VTL_scheduler_repo_Insert(&repo, &post);

    VTL_AppResult send = VTL_scheduler_dispatcher_Send(&repo, &post);
    TEST_ASSERT(send != VTL_res_kOk, "dispatcher_Send с невалидным metadata → ошибка");

    VTL_scheduler_Post fetched;
    memset(&fetched, 0, sizeof(fetched));
    VTL_scheduler_repo_GetById(&repo, post.id, &fetched);
    TEST_ASSERT(fetched.executed == false,
                "После ошибки dispatcher_Send запись остаётся executed = false");
    VTL_scheduler_post_Free(&fetched);

    VTL_scheduler_repo_Delete(&repo, post.id);
    VTL_scheduler_repo_Close(&repo);
}


static void test_tg_sched_send_null_post(void) {
    TEST_SECTION("dispatcher_Send — NULL post");

    VTL_scheduler_Repo repo;
    if (!test_repo_setup(&repo)) return;

    TEST_ASSERT(
            VTL_scheduler_dispatcher_Send(&repo, NULL) == VTL_res_kInvalidParamErr,
            "dispatcher_Send(repo, NULL) → kInvalidParamErr");

    VTL_scheduler_repo_Close(&repo);
}


static void test_tg_sched_send_already_executed(void) {
    TEST_SECTION("dispatcher_Send — пост уже executed");

    VTL_scheduler_Repo repo;
    if (!test_repo_setup(&repo)) return;

    VTL_scheduler_Post post;
    char *metadata = tg_sched_make_post(&post, "VTL re-send test", "");
    post.metadata = metadata;
    post.executed = true;

    VTL_scheduler_repo_Insert(&repo, &post);

    VTL_AppResult send = VTL_scheduler_dispatcher_Send(&repo, &post);
    TEST_ASSERT(send == VTL_res_kOk,
                "dispatcher_Send уже executed поста → kOk (повторная отправка)");

    VTL_scheduler_Post fetched;
    memset(&fetched, 0, sizeof(fetched));
    VTL_scheduler_repo_GetById(&repo, post.id, &fetched);
    TEST_ASSERT(fetched.executed == true,
                "Запись остаётся executed = true после повторной отправки");
    VTL_scheduler_post_Free(&fetched);

    VTL_scheduler_repo_Delete(&repo, post.id);
    free(metadata);
    VTL_scheduler_repo_Close(&repo);
}


static void test_tg_sched_send_empty_content(void) {
    TEST_SECTION("dispatcher_Send — TG пустой content");

    VTL_scheduler_Repo repo;
    if (!test_repo_setup(&repo)) return;

    VTL_scheduler_Post post;
    char *metadata = tg_sched_make_post(&post, "", "");
    post.metadata = metadata;

    VTL_scheduler_repo_Insert(&repo, &post);

    VTL_AppResult send = VTL_scheduler_dispatcher_Send(&repo, &post);
    TEST_ASSERT(send != VTL_res_kOk, "dispatcher_Send с пустым content → ошибка");

    VTL_scheduler_Post fetched;
    memset(&fetched, 0, sizeof(fetched));
    VTL_scheduler_repo_GetById(&repo, post.id, &fetched);
    TEST_ASSERT(fetched.executed == false,
                "После ошибки (пустой content) запись остаётся executed = false");
    VTL_scheduler_post_Free(&fetched);

    VTL_scheduler_repo_Delete(&repo, post.id);
    free(metadata);
    VTL_scheduler_repo_Close(&repo);
}


static void test_tg_sched_send_missing_chat_id(void) {
    TEST_SECTION("dispatcher_Send — TG отсутствует chat_id в metadata");

    VTL_scheduler_Repo repo;
    if (!test_repo_setup(&repo)) return;

    VTL_scheduler_Post post;
    memset(&post, 0, sizeof(post));
    post.send_date_time = time(NULL);
    post.created_user = "test_tg_no_cid";
    post.content_type = VTL_content_kTxt;
    post.content = "Текст без chat_id";
    post.sn_type = VTL_sn_kTG;
    post.metadata = "{\"parse_mode\":\"\"}";
    post.executed = false;

    VTL_scheduler_repo_Insert(&repo, &post);

    VTL_AppResult send = VTL_scheduler_dispatcher_Send(&repo, &post);
    TEST_ASSERT(send != VTL_res_kOk,
                "dispatcher_Send без chat_id в metadata → ошибка");

    VTL_scheduler_Post fetched;
    memset(&fetched, 0, sizeof(fetched));
    VTL_scheduler_repo_GetById(&repo, post.id, &fetched);
    TEST_ASSERT(fetched.executed == false,
                "После ошибки (нет chat_id) запись остаётся executed = false");
    VTL_scheduler_post_Free(&fetched);

    VTL_scheduler_repo_Delete(&repo, post.id);
    VTL_scheduler_repo_Close(&repo);
}


/* -----------------------------------------------------------------------
 * Suites
 * ----------------------------------------------------------------------- */
static void test_repo_suite(void) {
    printf("\n========================================\n");
    printf("  Тесты VTL_scheduler_repo\n");
    printf("========================================\n");
    fflush(stdout);

    g_tests_run = 0;
    g_tests_failed = 0;

    test_repo_open_close();
    test_repo_open_null_params();
    test_repo_ensure_table();
    test_repo_insert();
    test_repo_get_by_id();
    test_repo_update();
    test_repo_mark_executed();
    test_repo_delete();
    test_repo_get_pending();
    test_repo_insert_all_sn_types();
    test_repo_insert_content_types();

    printf("\n========================================\n");
    printf("  Итог: %d/%d тестов прошли",
           g_tests_run - g_tests_failed, g_tests_run);
    if (g_tests_failed == 0) {
        printf(" — все OK\n");
    } else {
        printf(", провалились: %d\n", g_tests_failed);
    }
    printf("========================================\n");
    fflush(stdout);
}


static void test_tg_text_suite(void) {
    printf("\n========================================\n");
    printf("  Тесты TG text send (через scheduler dispatcher)\n");
    printf("========================================\n");
    fflush(stdout);

    tg_sched_setup_env();

    g_tests_run = 0;
    g_tests_failed = 0;

    test_tg_sched_send_plain();
    test_tg_sched_send_markdownv2();
    test_tg_sched_send_unicode();
    test_tg_sched_send_long_text();
    test_tg_sched_send_bad_metadata();
    test_tg_sched_send_null_post();
    test_tg_sched_send_already_executed();
    test_tg_sched_send_empty_content();
    test_tg_sched_send_missing_chat_id();

    printf("\n========================================\n");
    printf("  Итог: %d/%d тестов прошли",
           g_tests_run - g_tests_failed, g_tests_run);
    if (g_tests_failed == 0) {
        printf(" — все OK\n");
    } else {
        printf(", провалились: %d\n", g_tests_failed);
    }
    printf("========================================\n");
    fflush(stdout);
}


/* -----------------------------------------------------------------------
 * Vimeo scheduler тесты
 * ----------------------------------------------------------------------- */
#define VIMEO_TEST_TOKEN    "23f05376b4302f9d0e7362fe316c46f7"
#define VIMEO_TEST_VIDEO    "ink_1.mp4"

static void vimeo_sched_setup_env(void) {
#ifndef _WIN32
    setenv("VIMEO_ACCESS_TOKEN", VIMEO_TEST_TOKEN, 1);
#else
    SetEnvironmentVariableA("VIMEO_ACCESS_TOKEN", VIMEO_TEST_TOKEN);
#endif
}

static char *vimeo_sched_make_metadata(const char *title,
                                       const char *description,
                                       const char *tags_csv) {
    VTL_scheduler_MetaVimeo meta;
    memset(&meta, 0, sizeof(meta));
    if (title) strncpy(meta.title, title, sizeof(meta.title) - 1);
    if (description) strncpy(meta.description, description, sizeof(meta.description) - 1);
    if (tags_csv) strncpy(meta.tags_csv, tags_csv, sizeof(meta.tags_csv) - 1);
    return VTL_scheduler_meta_SerializeVimeo(&meta);
}


/* --- Сериализация / десериализация метаданных -------------------------- */

static void test_vimeo_meta_serialize_deserialize(void) {
    TEST_SECTION("Vimeo meta — Serialize / Deserialize");

    VTL_scheduler_MetaVimeo src;
    memset(&src, 0, sizeof(src));
    strncpy(src.title, "Тест VTL", sizeof(src.title) - 1);
    strncpy(src.description, "Описание видео", sizeof(src.description) - 1);
    strncpy(src.tags_csv, "vtl,test,video", sizeof(src.tags_csv) - 1);

    char *json = VTL_scheduler_meta_SerializeVimeo(&src);
    TEST_ASSERT(json != NULL, "SerializeVimeo возвращает не-NULL");

    VTL_scheduler_MetaVimeo dst;
    VTL_AppResult res = VTL_scheduler_meta_DeserializeVimeo(json, &dst);
    TEST_ASSERT(res == VTL_res_kOk, "DeserializeVimeo возвращает kOk");
    TEST_ASSERT(strcmp(dst.title, src.title) == 0, "title  совпадает после round-trip");
    TEST_ASSERT(strcmp(dst.description, src.description) == 0, "description совпадает после round-trip");
    TEST_ASSERT(strcmp(dst.tags_csv, src.tags_csv) == 0, "tags_csv совпадает после round-trip");

    free(json);
}

static void test_vimeo_meta_null_params(void) {
    TEST_SECTION("Vimeo meta — NULL-аргументы");

    TEST_ASSERT(VTL_scheduler_meta_DeserializeVimeo(NULL, NULL) == VTL_res_kInvalidParamErr,
                "DeserializeVimeo(NULL, NULL) → kInvalidParamErr");

    VTL_scheduler_MetaVimeo out;
    TEST_ASSERT(VTL_scheduler_meta_DeserializeVimeo(NULL, &out) == VTL_res_kInvalidParamErr,
                "DeserializeVimeo(NULL, out) → kInvalidParamErr");
    TEST_ASSERT(VTL_scheduler_meta_DeserializeVimeo("{}", NULL) == VTL_res_kInvalidParamErr,
                "DeserializeVimeo(json, NULL) → kInvalidParamErr");

    TEST_ASSERT(VTL_scheduler_meta_SerializeVimeo(NULL) == NULL,
                "SerializeVimeo(NULL) → NULL");
}

static void test_vimeo_meta_empty_fields(void) {
    TEST_SECTION("Vimeo meta — пустые поля");

    char *json = vimeo_sched_make_metadata("", "", "");
    TEST_ASSERT(json != NULL, "SerializeVimeo с пустыми полями возвращает не-NULL");

    VTL_scheduler_MetaVimeo dst;
    VTL_AppResult res = VTL_scheduler_meta_DeserializeVimeo(json, &dst);
    TEST_ASSERT(res == VTL_res_kOk, "DeserializeVimeo с пустыми полями → kOk");
    TEST_ASSERT(dst.title[0] == '\0', "title пустой");
    TEST_ASSERT(dst.description[0] == '\0', "description пустой");
    TEST_ASSERT(dst.tags_csv[0] == '\0', "tags_csv пустой");

    free(json);
}

static void test_vimeo_meta_invalid_json(void) {
    TEST_SECTION("Vimeo meta — невалидный JSON");

    VTL_scheduler_MetaVimeo out;
    TEST_ASSERT(VTL_scheduler_meta_DeserializeVimeo("{ not json !!!", &out) != VTL_res_kOk,
                "DeserializeVimeo невалидного JSON → ошибка");
}

static void test_vimeo_sched_send_video_only(void) {
    TEST_SECTION("dispatcher_Send — Vimeo видео без метаданных");

    VTL_scheduler_Repo repo;
    if (!test_repo_setup(&repo)) return;

    if (access(VIMEO_TEST_VIDEO, F_OK) != 0) {
        printf("  Файл %s не найден — тест пропускается.\n", VIMEO_TEST_VIDEO);
        fflush(stdout);
        VTL_scheduler_repo_Close(&repo);
        return;
    }

    char *metadata = vimeo_sched_make_metadata("", "", "");

    VTL_scheduler_Post post;
    memset(&post, 0, sizeof(post));
    post.send_date_time = time(NULL);
    post.created_user = "test_vimeo";
    post.content_type = VTL_content_kFilePath;
    post.content = VIMEO_TEST_VIDEO;
    post.sn_type = VTL_sn_kVimeo;
    post.metadata = metadata;
    post.executed = false;

    VTL_scheduler_repo_Insert(&repo, &post);

    VTL_AppResult res = VTL_scheduler_dispatcher_Send(&repo, &post);
    TEST_ASSERT(res == VTL_res_kOk, "dispatcher_Send Vimeo (только видео) → kOk");

    VTL_scheduler_Post fetched;
    memset(&fetched, 0, sizeof(fetched));
    VTL_scheduler_repo_GetById(&repo, post.id, &fetched);
    TEST_ASSERT(fetched.executed == true,
                "После отправки на Vimeo запись помечена executed = true");
    VTL_scheduler_post_Free(&fetched);

    VTL_scheduler_repo_Delete(&repo, post.id);
    free(metadata);
    VTL_scheduler_repo_Close(&repo);
}

static void test_vimeo_sched_send_with_description(void) {
    TEST_SECTION("dispatcher_Send — Vimeo видео с description");

    VTL_scheduler_Repo repo;
    if (!test_repo_setup(&repo)) return;

    if (access(VIMEO_TEST_VIDEO, F_OK) != 0) {
        printf("  Файл %s не найден — тест пропускается.\n", VIMEO_TEST_VIDEO);
        fflush(stdout);
        VTL_scheduler_repo_Close(&repo);
        return;
    }

    char *metadata = vimeo_sched_make_metadata(
            "VTL scheduler test",
            "Тест загрузки видео через VTL scheduler dispatcher. Описание на русском.",
            "");

    VTL_scheduler_Post post;
    memset(&post, 0, sizeof(post));
    post.send_date_time = time(NULL);
    post.created_user = "test_vimeo_desc";
    post.content_type = VTL_content_kFilePath;
    post.content = VIMEO_TEST_VIDEO;
    post.sn_type = VTL_sn_kVimeo;
    post.metadata = metadata;
    post.executed = false;

    VTL_scheduler_repo_Insert(&repo, &post);

    VTL_AppResult res = VTL_scheduler_dispatcher_Send(&repo, &post);
    TEST_ASSERT(res == VTL_res_kOk, "dispatcher_Send Vimeo с description → kOk");

    VTL_scheduler_Post fetched;
    memset(&fetched, 0, sizeof(fetched));
    VTL_scheduler_repo_GetById(&repo, post.id, &fetched);
    TEST_ASSERT(fetched.executed == true,
                "После отправки с description запись помечена executed = true");
    VTL_scheduler_post_Free(&fetched);

    VTL_scheduler_repo_Delete(&repo, post.id);
    free(metadata);
    VTL_scheduler_repo_Close(&repo);
}

static void test_vimeo_sched_send_with_full_meta(void) {
    TEST_SECTION("dispatcher_Send — Vimeo видео с title + description + tags");

    VTL_scheduler_Repo repo;
    if (!test_repo_setup(&repo)) return;

    if (access(VIMEO_TEST_VIDEO, F_OK) != 0) {
        printf("  Файл %s не найден — тест пропускается.\n", VIMEO_TEST_VIDEO);
        fflush(stdout);
        VTL_scheduler_repo_Close(&repo);
        return;
    }

    char *metadata = vimeo_sched_make_metadata(
            "VTL full meta test",
            "Полный набор метаданных: title, description, tags.",
            "vtl,scheduler,vimeo,test");

    VTL_scheduler_Post post;
    memset(&post, 0, sizeof(post));
    post.send_date_time = time(NULL);
    post.created_user = "test_vimeo_full";
    post.content_type = VTL_content_kFilePath;
    post.content = VIMEO_TEST_VIDEO;
    post.sn_type = VTL_sn_kVimeo;
    post.metadata = metadata;
    post.executed = false;

    VTL_scheduler_repo_Insert(&repo, &post);

    VTL_AppResult res = VTL_scheduler_dispatcher_Send(&repo, &post);
    TEST_ASSERT(res == VTL_res_kOk, "dispatcher_Send Vimeo с полными метаданными → kOk");

    VTL_scheduler_Post fetched;
    memset(&fetched, 0, sizeof(fetched));
    VTL_scheduler_repo_GetById(&repo, post.id, &fetched);
    TEST_ASSERT(fetched.executed == true,
                "После отправки с полными метаданными запись помечена executed = true");
    VTL_scheduler_post_Free(&fetched);

    VTL_scheduler_repo_Delete(&repo, post.id);
    free(metadata);
    VTL_scheduler_repo_Close(&repo);
}

static void test_vimeo_sched_send_empty_path(void) {
    TEST_SECTION("dispatcher_Send — Vimeo пустой путь к файлу");

    VTL_scheduler_Repo repo;
    if (!test_repo_setup(&repo)) return;

    char *metadata = vimeo_sched_make_metadata("", "", "");

    VTL_scheduler_Post post;
    memset(&post, 0, sizeof(post));
    post.send_date_time = time(NULL);
    post.created_user = "test_vimeo_empty";
    post.content_type = VTL_content_kFilePath;
    post.content = "";
    post.sn_type = VTL_sn_kVimeo;
    post.metadata = metadata;
    post.executed = false;

    VTL_scheduler_repo_Insert(&repo, &post);

    VTL_AppResult res = VTL_scheduler_dispatcher_Send(&repo, &post);
    TEST_ASSERT(res != VTL_res_kOk, "dispatcher_Send Vimeo с пустым путём → ошибка");

    VTL_scheduler_Post fetched;
    memset(&fetched, 0, sizeof(fetched));
    VTL_scheduler_repo_GetById(&repo, post.id, &fetched);
    TEST_ASSERT(fetched.executed == false,
                "После ошибки (пустой путь) запись остаётся executed = false");
    VTL_scheduler_post_Free(&fetched);

    VTL_scheduler_repo_Delete(&repo, post.id);
    free(metadata);
    VTL_scheduler_repo_Close(&repo);
}

static void test_vimeo_sched_send_bad_metadata(void) {
    TEST_SECTION("dispatcher_Send — Vimeo невалидный metadata JSON");

    VTL_scheduler_Repo repo;
    if (!test_repo_setup(&repo)) return;

    VTL_scheduler_Post post;
    memset(&post, 0, sizeof(post));
    post.send_date_time = time(NULL);
    post.created_user = "test_vimeo_badmeta";
    post.content_type = VTL_content_kFilePath;
    post.content = VIMEO_TEST_VIDEO;
    post.sn_type = VTL_sn_kVimeo;
    post.metadata = "{ not valid json !!!";
    post.executed = false;

    VTL_scheduler_repo_Insert(&repo, &post);

    VTL_AppResult res = VTL_scheduler_dispatcher_Send(&repo, &post);
    TEST_ASSERT(res != VTL_res_kOk, "dispatcher_Send Vimeo с невалидным JSON → ошибка");

    VTL_scheduler_Post fetched;
    memset(&fetched, 0, sizeof(fetched));
    VTL_scheduler_repo_GetById(&repo, post.id, &fetched);
    TEST_ASSERT(fetched.executed == false,
                "После ошибки (плохой JSON) запись остаётся executed = false");
    VTL_scheduler_post_Free(&fetched);

    VTL_scheduler_repo_Delete(&repo, post.id);
    VTL_scheduler_repo_Close(&repo);
}

static void test_vimeo_sched_send_null_post(void) {
    TEST_SECTION("dispatcher_Send — Vimeo NULL post");

    VTL_scheduler_Repo repo;
    if (!test_repo_setup(&repo)) return;

    TEST_ASSERT(
            VTL_scheduler_dispatcher_Send(&repo, NULL) == VTL_res_kInvalidParamErr,
            "dispatcher_Send(repo, NULL) → kInvalidParamErr");

    VTL_scheduler_repo_Close(&repo);
}

static void test_vimeo_sched_repo_sn_type_stored(void) {
    TEST_SECTION("scheduler_repo — sn_type kVimeo сохраняется и читается");

    VTL_scheduler_Repo repo;
    if (!test_repo_setup(&repo)) return;

    char *metadata = vimeo_sched_make_metadata("Store test", "", "");

    VTL_scheduler_Post post;
    memset(&post, 0, sizeof(post));
    post.send_date_time = time(NULL) + 60;
    post.created_user = "test_vimeo_sntype";
    post.content_type = VTL_content_kFilePath;
    post.content = VIMEO_TEST_VIDEO;
    post.sn_type = VTL_sn_kVimeo;
    post.metadata = metadata;
    post.executed = false;

    VTL_AppResult ins = VTL_scheduler_repo_Insert(&repo, &post);
    TEST_ASSERT(ins == VTL_res_kOk, "Insert Vimeo-поста → kOk");
    TEST_ASSERT(post.id > 0, "Insert заполняет id > 0");

    VTL_scheduler_Post fetched;
    memset(&fetched, 0, sizeof(fetched));
    VTL_scheduler_repo_GetById(&repo, post.id, &fetched);
    TEST_ASSERT(fetched.sn_type == VTL_sn_kVimeo,
                "GetById возвращает sn_type == kVimeo");
    TEST_ASSERT(fetched.content_type == VTL_content_kFilePath,
                "GetById возвращает content_type == kFilePath");

    VTL_scheduler_MetaVimeo meta_back;
    VTL_AppResult dr = VTL_scheduler_meta_DeserializeVimeo(fetched.metadata, &meta_back);
    TEST_ASSERT(dr == VTL_res_kOk, "metadata из БД десериализуется без ошибок");
    TEST_ASSERT(strcmp(meta_back.title, "Store test") == 0,
                "title из metadata совпадает с сохранённым");

    VTL_scheduler_post_Free(&fetched);
    VTL_scheduler_repo_Delete(&repo, post.id);
    free(metadata);
    VTL_scheduler_repo_Close(&repo);
}


static void test_vimeo_suite(void) {
    printf("\n========================================\n");
    printf("  Тесты Vimeo scheduler\n");
    printf("========================================\n");
    fflush(stdout);

    vimeo_sched_setup_env();

    g_tests_run = 0;
    g_tests_failed = 0;

    /* Метаданные — не требуют БД и сети */
    test_vimeo_meta_serialize_deserialize();
    test_vimeo_meta_null_params();
    test_vimeo_meta_empty_fields();
    test_vimeo_meta_invalid_json();

    /* dispatcher + repo — требуют БД и сети */
    test_vimeo_sched_repo_sn_type_stored();
    test_vimeo_sched_send_null_post();
    test_vimeo_sched_send_empty_path();
    test_vimeo_sched_send_bad_metadata();
    test_vimeo_sched_send_video_only();
    test_vimeo_sched_send_with_description();
    test_vimeo_sched_send_with_full_meta();

    printf("\n========================================\n");
    printf("  Итог: %d/%d тестов прошли",
           g_tests_run - g_tests_failed, g_tests_run);
    if (g_tests_failed == 0) {
        printf(" — все OK\n");
    } else {
        printf(", провалились: %d\n", g_tests_failed);
    }
    printf("========================================\n");
    fflush(stdout);
}


int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    test_repo_suite();
    test_tg_text_suite();
    test_vimeo_suite();

    return 0;
}