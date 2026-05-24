#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif
#endif

#include <VTL/scheduler/db/VTL_scheduler_db_repo.h>
#include <VTL/utils/db/VTL_utils_db_connection.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#ifdef _MSC_VER
/* _strdup — точный аналог strdup в MS-CRT (POSIX-имя deprecated).
 * sscanf_s НЕ переименовываем через макрос: для %s/%c/%[ у него
 * другая сигнатура (нужен размер буфера) — мина при добавлении
 * новых форматов. Используем явно в местах вызова. */
#  define strdup _strdup
#endif


static VTL_scheduler_SnType sn_from_str(const char *s) {
    if (!s) return VTL_sn_kUnknown;
    if (strcmp(s, "TG") == 0)     return VTL_sn_kTG;
    if (strcmp(s, "REDDIT") == 0) return VTL_sn_kReddit;
    if (strcmp(s, "VK") == 0)     return VTL_sn_kVK;
    if (strcmp(s, "VIMEO") == 0)  return VTL_sn_kVimeo;
    return VTL_sn_kUnknown;
}

static const char *sn_to_str(VTL_scheduler_SnType t) {
    switch (t) {
        case VTL_sn_kTG:     return "TG";
        case VTL_sn_kReddit: return "REDDIT";
        case VTL_sn_kVK:     return "VK";
        case VTL_sn_kVimeo:  return "VIMEO";
        default:             return "UNKNOWN";
    }
}

static VTL_scheduler_ContentType ct_from_str(const char *s) {
    if (!s) return VTL_content_kUnknown;
    if (strcmp(s, "TXT") == 0) return VTL_content_kTxt;
    if (strcmp(s, "FILE_PATH") == 0) return VTL_content_kFilePath;
    return VTL_content_kUnknown;
}

static const char *ct_to_str(VTL_scheduler_ContentType t) {
    switch (t) {
        case VTL_content_kTxt:
            return "TXT";
        case VTL_content_kFilePath:
            return "FILE_PATH";
        default:
            return "TXT";
    }
}

static void ts_to_iso(time_t ts, char *buf, size_t sz) {
    struct tm tm_utc;
#ifdef _WIN32
    gmtime_s(&tm_utc, &ts);
#else
    gmtime_r(&ts, &tm_utc);
#endif
    strftime(buf, sz, "%Y-%m-%d %H:%M:%S", &tm_utc);
}

static time_t iso_to_ts(const char *s) {
    if (!s || *s == '\0') return 0;
    struct tm tm_val;
    memset(&tm_val, 0, sizeof(tm_val));
#ifdef _WIN32
    /* strptime не входит в MSVC CRT — берём sscanf. На MSVC sscanf
     * deprecated → sscanf_s; для целочисленных %d сигнатура та же. */
#  ifdef _MSC_VER
    sscanf_s(s, "%d-%d-%d %d:%d:%d",
             &tm_val.tm_year, &tm_val.tm_mon, &tm_val.tm_mday,
             &tm_val.tm_hour, &tm_val.tm_min, &tm_val.tm_sec);
#  else
    sscanf(s, "%d-%d-%d %d:%d:%d",
           &tm_val.tm_year, &tm_val.tm_mon, &tm_val.tm_mday,
           &tm_val.tm_hour, &tm_val.tm_min, &tm_val.tm_sec);
#  endif
    tm_val.tm_year -= 1900;
    tm_val.tm_mon  -= 1;
    return _mkgmtime(&tm_val);
#else
    strptime(s, "%Y-%m-%d %H:%M:%S", &tm_val);
    return timegm(&tm_val);
#endif
}

static char *pq_strdup(PGresult *res, int row, int col) {
    if (PQgetisnull(res, row, col)) return NULL;
    return strdup(PQgetvalue(res, row, col));
}

static void row_to_post(PGresult *res, int row, VTL_scheduler_Post *p) {
    memset(p, 0, sizeof(*p));
    p->id = atoll(PQgetvalue(res, row, 0));
    p->send_date_time = iso_to_ts(PQgetvalue(res, row, 1));
    p->created_user = pq_strdup(res, row, 2);
    p->content_type = ct_from_str(PQgetvalue(res, row, 3));
    p->content = pq_strdup(res, row, 4);
    p->sn_type = sn_from_str(PQgetvalue(res, row, 5));
    p->metadata = pq_strdup(res, row, 6);
    const char *ex = PQgetvalue(res, row, 7);
    p->executed = (ex && (ex[0] == 't' || ex[0] == '1'));
}


VTL_AppResult VTL_scheduler_repo_Open(VTL_scheduler_Repo *repo,
                                      VTL_db_Credentals *creds) {
    if (!repo || !creds) return VTL_res_kInvalidParamErr;

    repo->conn = VTL_connect_to_db(creds);

    if (!repo->conn || PQstatus(repo->conn) != CONNECTION_OK) {
        fprintf(stderr, "[scheduler_repo] connection failed: %s\n",
                repo->conn ? PQerrorMessage(repo->conn) : "null");
        return VTL_res_kErr;
    }
    return VTL_res_kOk;
}

void VTL_scheduler_repo_Close(VTL_scheduler_Repo *repo) {
    if (!repo) return;
    if (repo->conn) {
        PQfinish(repo->conn);
        repo->conn = NULL;
    }
}


VTL_AppResult VTL_scheduler_repo_EnsureTable(VTL_scheduler_Repo *repo) {
    if (!repo || !repo->conn) return VTL_res_kInvalidParamErr;

    const char *sql =
            "CREATE TABLE IF NOT EXISTS scheduled_posts ("
            "  id              BIGSERIAL PRIMARY KEY,"
            "  send_date_time  TIMESTAMP NOT NULL,"
            "  created_user    TEXT NOT NULL,"
            "  content_type    VARCHAR(16) NOT NULL DEFAULT 'TXT',"
            "  content         TEXT NOT NULL,"
            "  sn_type         VARCHAR(16) NOT NULL,"
            "  metadata        TEXT,"
            "  executed        BOOLEAN NOT NULL DEFAULT FALSE"
            ");";

    PGresult *res = PQexec(repo->conn, sql);
    ExecStatusType st = PQresultStatus(res);
    PQclear(res);

    if (st != PGRES_COMMAND_OK) {
        fprintf(stderr, "[scheduler_repo] EnsureTable failed: %s\n",
                PQerrorMessage(repo->conn));
        return VTL_res_kErr;
    }
    return VTL_res_kOk;
}


VTL_AppResult VTL_scheduler_repo_Insert(VTL_scheduler_Repo *repo,
                                        VTL_scheduler_Post *post) {
    if (!repo || !repo->conn || !post) return VTL_res_kInvalidParamErr;

    char dt_buf[32];
    ts_to_iso(post->send_date_time, dt_buf, sizeof(dt_buf));

    const char *params[7];
    params[0] = dt_buf;
    params[1] = post->created_user ? post->created_user : "";
    params[2] = ct_to_str(post->content_type);
    params[3] = post->content ? post->content : "";
    params[4] = sn_to_str(post->sn_type);
    params[5] = post->metadata ? post->metadata : "";
    params[6] = post->executed ? "true" : "false";

    const char *sql =
            "INSERT INTO scheduled_posts"
            " (send_date_time, created_user, content_type, content,"
            "  sn_type, metadata, executed)"
            " VALUES ($1::TIMESTAMP, $2, $3, $4, $5, $6, $7::BOOLEAN)"
            " RETURNING id;";

    PGresult *res = PQexecParams(repo->conn, sql,
                                 7, NULL, params, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "[scheduler_repo] Insert failed: %s\n",
                PQerrorMessage(repo->conn));
        PQclear(res);
        return VTL_res_kErr;
    }

    post->id = atoll(PQgetvalue(res, 0, 0));
    PQclear(res);
    return VTL_res_kOk;
}


VTL_AppResult VTL_scheduler_repo_GetById(VTL_scheduler_Repo *repo,
                                         long long id,
                                         VTL_scheduler_Post *out) {
    if (!repo || !repo->conn || !out) return VTL_res_kInvalidParamErr;

    char id_str[32];
    snprintf(id_str, sizeof(id_str), "%lld", id);
    const char *params[] = {id_str};

    const char *sql =
            "SELECT id, to_char(send_date_time,'YYYY-MM-DD HH24:MI:SS'),"
            "       created_user, content_type, content,"
            "       sn_type, metadata, executed"
            " FROM scheduled_posts WHERE id = $1::BIGINT;";

    PGresult *res = PQexecParams(repo->conn, sql,
                                 1, NULL, params, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        fprintf(stderr, "[scheduler_repo] GetById(%lld) not found\n", id);
        PQclear(res);
        return VTL_res_kErr;
    }

    row_to_post(res, 0, out);
    PQclear(res);
    return VTL_res_kOk;
}


VTL_AppResult VTL_scheduler_repo_GetPending(VTL_scheduler_Repo *repo,
                                            int lookahead_sec,
                                            VTL_scheduler_PostList *list) {
    if (!repo || !repo->conn || !list) return VTL_res_kInvalidParamErr;

    char la_str[16];
    snprintf(la_str, sizeof(la_str), "%d", lookahead_sec);
    const char *params[] = {la_str};

    const char *sql =
            "SELECT id, to_char(send_date_time,'YYYY-MM-DD HH24:MI:SS'),"
            "       created_user, content_type, content,"
            "       sn_type, metadata, executed"
            " FROM scheduled_posts"
            " WHERE executed = FALSE"
            "   AND send_date_time <= NOW() + ($1::INTEGER * INTERVAL '1 second')"
            " ORDER BY send_date_time ASC;";

    PGresult *res = PQexecParams(repo->conn, sql,
                                 1, NULL, params, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "[scheduler_repo] GetPending failed: %s\n",
                PQerrorMessage(repo->conn));
        PQclear(res);
        return VTL_res_kErr;
    }

    int n = PQntuples(res);
    list->items = (VTL_scheduler_Post *) calloc((size_t) n, sizeof(VTL_scheduler_Post));
    list->length = 0;
    list->capacity = (size_t) n;

    if (n > 0 && !list->items) {
        PQclear(res);
        return VTL_res_kMemAllocErr;
    }

    for (int i = 0; i < n; ++i) {
        row_to_post(res, i, &list->items[i]);
        list->length++;
    }

    PQclear(res);
    return VTL_res_kOk;
}


VTL_AppResult VTL_scheduler_repo_Update(VTL_scheduler_Repo *repo,
                                        const VTL_scheduler_Post *post) {
    if (!repo || !repo->conn || !post) return VTL_res_kInvalidParamErr;

    char dt_buf[32];
    ts_to_iso(post->send_date_time, dt_buf, sizeof(dt_buf));

    char id_str[32];
    snprintf(id_str, sizeof(id_str), "%lld", post->id);

    const char *params[8];
    params[0] = dt_buf;
    params[1] = post->created_user ? post->created_user : "";
    params[2] = ct_to_str(post->content_type);
    params[3] = post->content ? post->content : "";
    params[4] = sn_to_str(post->sn_type);
    params[5] = post->metadata ? post->metadata : "";
    params[6] = post->executed ? "true" : "false";
    params[7] = id_str;

    const char *sql =
            "UPDATE scheduled_posts SET"
            "  send_date_time = $1::TIMESTAMP,"
            "  created_user   = $2,"
            "  content_type   = $3,"
            "  content        = $4,"
            "  sn_type        = $5,"
            "  metadata       = $6,"
            "  executed       = $7::BOOLEAN"
            " WHERE id = $8::BIGINT;";

    PGresult *res = PQexecParams(repo->conn, sql,
                                 8, NULL, params, NULL, NULL, 0);
    ExecStatusType st = PQresultStatus(res);
    PQclear(res);

    if (st != PGRES_COMMAND_OK) {
        fprintf(stderr, "[scheduler_repo] Update failed: %s\n",
                PQerrorMessage(repo->conn));
        return VTL_res_kErr;
    }
    return VTL_res_kOk;
}


VTL_AppResult VTL_scheduler_repo_MarkExecuted(VTL_scheduler_Repo *repo,
                                              long long id) {
    if (!repo || !repo->conn) return VTL_res_kInvalidParamErr;

    char id_str[32];
    snprintf(id_str, sizeof(id_str), "%lld", id);
    const char *params[] = {id_str};

    const char *sql =
            "UPDATE scheduled_posts SET executed = TRUE"
            " WHERE id = $1::BIGINT;";

    PGresult *res = PQexecParams(repo->conn, sql,
                                 1, NULL, params, NULL, NULL, 0);
    ExecStatusType st = PQresultStatus(res);
    PQclear(res);

    if (st != PGRES_COMMAND_OK) {
        fprintf(stderr, "[scheduler_repo] MarkExecuted(%lld) failed: %s\n",
                id, PQerrorMessage(repo->conn));
        return VTL_res_kErr;
    }
    return VTL_res_kOk;
}


VTL_AppResult VTL_scheduler_repo_Delete(VTL_scheduler_Repo *repo,
                                        long long id) {
    if (!repo || !repo->conn) return VTL_res_kInvalidParamErr;

    char id_str[32];
    snprintf(id_str, sizeof(id_str), "%lld", id);
    const char *params[] = {id_str};

    const char *sql =
            "DELETE FROM scheduled_posts WHERE id = $1::BIGINT;";

    PGresult *res = PQexecParams(repo->conn, sql,
                                 1, NULL, params, NULL, NULL, 0);
    ExecStatusType st = PQresultStatus(res);
    PQclear(res);

    if (st != PGRES_COMMAND_OK) {
        fprintf(stderr, "[scheduler_repo] Delete(%lld) failed: %s\n",
                id, PQerrorMessage(repo->conn));
        return VTL_res_kErr;
    }
    return VTL_res_kOk;
}


void VTL_scheduler_post_Free(VTL_scheduler_Post *p) {
    if (!p) return;
    free(p->created_user);
    p->created_user = NULL;
    free(p->content);
    p->content = NULL;
    free(p->metadata);
    p->metadata = NULL;
}

void VTL_scheduler_postlist_Free(VTL_scheduler_PostList *list) {
    if (!list) return;
    for (size_t i = 0; i < list->length; ++i)
        VTL_scheduler_post_Free(&list->items[i]);
    free(list->items);
    list->items = NULL;
    list->length = 0;
    list->capacity = 0;
}