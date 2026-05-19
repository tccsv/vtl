#include <VTL/history_administration/db/VTL_history_administration_read.h>
#include <VTL/utils/db/VTL_db.h>
#include <VTL/utils/VTL_time.h>
#include <VTL/history_administration/VTL_history_administration_data.h>
#include <libpq-fe.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

static void time_to_sql(const VTL_Time* p_time, char* buf, size_t buf_size) {
    time_t raw = (time_t)(*p_time);
    struct tm* tm_info = localtime(&raw);
    strftime(buf, buf_size, "%Y-%m-%d %H:%M:%S", tm_info);
}

static const char* platform_str(int platform) {
    switch (platform) {
        case VTL_PLATFORM_TELEGRAM: return "TG";
        case VTL_PLATFORM_VK:       return "VK";
        default:                    return "??";
    }
}

static const char* status_str(int status) {
    switch (status) {
        case VTL_PUBLICATION_DRAFT:     return "Draft";
        case VTL_PUBLICATION_SCHEDULED: return "Scheduled";
        case VTL_PUBLICATION_PUBLISHED: return "Published";
        case VTL_PUBLICATION_FAILED:    return "Failed";
        default:                        return "Unknown";
    }
}

static void print_row(PGresult* res, int i) {
    int plat = atoi(PQgetvalue(res, i, 3));
    int stat = atoi(PQgetvalue(res, i, 4));

    printf("  #%-4s | @%-15s | %-3s | %-10s | %s",
        PQgetvalue(res, i, 0),
        PQgetvalue(res, i, 1),
        platform_str(plat),
        status_str(stat),
        PQgetvalue(res, i, 7));

    if (!PQgetisnull(res, i, 5)) {
        printf(" | text: %s", PQgetvalue(res, i, 5));
    }
    if (!PQgetisnull(res, i, 6)) {
        printf(" | media: %s", PQgetvalue(res, i, 6));
    }
    printf("\n");
}

static const char* BASE_SELECT =
    "SELECT id, user_nickname, publication_type, platform, "
    "status, text_file_name, media_file_name, published_at "
    "FROM publication_history";

static VTL_AppResult execute_and_print(VTL_Database* db, const char* sql, const char* title) {
    PGresult* res = PQexec(db->conn, sql);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        printf("Query failed: %s\n", PQerrorMessage(db->conn));
        PQclear(res);
        return VTL_res_kErr;
    }

    int rows = PQntuples(res);
    printf("\n=== %s (%d) ===\n", title, rows);

    if (rows == 0) {
        printf("  (no records)\n");
    }

    for (int i = 0; i < rows; i++) {
        print_row(res, i);
    }

    PQclear(res);
    return VTL_res_kOk;
}

// ==================== ShowAll ====================

VTL_AppResult VTL_history_administration_ShowAll(VTL_Database* db) {
    char sql[512];
    snprintf(sql, sizeof(sql), "%s ORDER BY published_at DESC", BASE_SELECT);
    return execute_and_print(db, sql, "All publications");
}

// ==================== ShowByUser ====================

VTL_AppResult VTL_history_administration_ShowByUser(VTL_Database* db, const char* user_name) {
    char sql[512];
    snprintf(sql, sizeof(sql),
        "%s WHERE user_nickname = '%s' ORDER BY published_at DESC",
        BASE_SELECT, user_name);

    char title[128];
    snprintf(title, sizeof(title), "Publications by @%s", user_name);

    return execute_and_print(db, sql, title);
}

// ==================== ShowBeforeTime ====================

VTL_AppResult VTL_history_administration_ShowBeforeTime(VTL_Database* db, const VTL_Time* p_time) {
    char time_buf[64];
    time_to_sql(p_time, time_buf, sizeof(time_buf));

    char sql[512];
    snprintf(sql, sizeof(sql),
        "%s WHERE published_at < '%s' ORDER BY published_at DESC",
        BASE_SELECT, time_buf);

    char title[128];
    snprintf(title, sizeof(title), "Publications before %s", time_buf);

    return execute_and_print(db, sql, title);
}

// ==================== ShowAfterTime ====================

VTL_AppResult VTL_history_administration_ShowAfterTime(VTL_Database* db, const VTL_Time* p_time) {
    char time_buf[64];
    time_to_sql(p_time, time_buf, sizeof(time_buf));

    char sql[512];
    snprintf(sql, sizeof(sql),
        "%s WHERE published_at > '%s' ORDER BY published_at DESC",
        BASE_SELECT, time_buf);

    char title[128];
    snprintf(title, sizeof(title), "Publications after %s", time_buf);

    return execute_and_print(db, sql, title);
}

// ==================== ShowByPlatform ====================

VTL_AppResult VTL_history_administration_ShowByPlatform(VTL_Database* db, VTL_Platform platform) {
    char sql[512];
    snprintf(sql, sizeof(sql),
        "%s WHERE platform = %d ORDER BY published_at DESC",
        BASE_SELECT, platform);

    char title[128];
    snprintf(title, sizeof(title), "Publications on %s",
        platform == VTL_PLATFORM_TELEGRAM ? "Telegram" : "VK");

    return execute_and_print(db, sql, title);
}

// ==================== ShowByStatus ====================

VTL_AppResult VTL_history_administration_ShowByStatus(VTL_Database* db, VTL_PublicationStatus status) {
    char sql[512];
    snprintf(sql, sizeof(sql),
        "%s WHERE status = %d ORDER BY published_at DESC",
        BASE_SELECT, status);

    char title[128];
    snprintf(title, sizeof(title), "Publications with status %s", status_str(status));

    return execute_and_print(db, sql, title);
}

// ==================== FindById ====================

VTL_AppResult VTL_history_administration_FindById(VTL_Database* db, int id, VTL_UserHistory* history) {
    char sql[512];
    snprintf(sql, sizeof(sql),
        "%s WHERE id = %d", BASE_SELECT, id);

    PGresult* res = PQexec(db->conn, sql);

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        printf("Publication #%d not found.\n", id);
        PQclear(res);
        return VTL_res_kErr;
    }

    memset(history, 0, sizeof(VTL_UserHistory));
    history->id = atoi(PQgetvalue(res, 0, 0));
    snprintf(history->user.nickname, sizeof(history->user.nickname), "%s", PQgetvalue(res, 0, 1));
    history->publication_type = atoi(PQgetvalue(res, 0, 2));
    history->platform = atoi(PQgetvalue(res, 0, 3));
    history->status = atoi(PQgetvalue(res, 0, 4));

    if (!PQgetisnull(res, 0, 5)) {
        snprintf(history->text_file_name, sizeof(history->text_file_name), "%s", PQgetvalue(res, 0, 5));
        history->has_text_file = true;
    }
    if (!PQgetisnull(res, 0, 6)) {
        snprintf(history->media_file_name, sizeof(history->media_file_name), "%s", PQgetvalue(res, 0, 6));
        history->has_media_file = true;
    }

    PQclear(res);
    return VTL_res_kOk;
}

// ==================== DownloadAll ====================

VTL_AppResult VTL_history_administration_DownloadAll(VTL_Database* db) {
    char sql[512];
    snprintf(sql, sizeof(sql), "%s ORDER BY published_at DESC", BASE_SELECT);

    PGresult* res = PQexec(db->conn, sql);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        printf("Query failed: %s\n", PQerrorMessage(db->conn));
        PQclear(res);
        return VTL_res_kErr;
    }

    FILE* file = fopen("publication_history_export.csv", "w");
    if (!file) {
        printf("Cannot create export file.\n");
        PQclear(res);
        return VTL_res_kErr;
    }

    fprintf(file, "id,user,type,platform,status,text_file,media_file,published_at\n");

    int rows = PQntuples(res);
    for (int i = 0; i < rows; i++) {
        fprintf(file, "%s,%s,%s,%s,%s,%s,%s,%s\n",
            PQgetvalue(res, i, 0),
            PQgetvalue(res, i, 1),
            PQgetvalue(res, i, 2),
            PQgetvalue(res, i, 3),
            PQgetvalue(res, i, 4),
            PQgetisnull(res, i, 5) ? "" : PQgetvalue(res, i, 5),
            PQgetisnull(res, i, 6) ? "" : PQgetvalue(res, i, 6),
            PQgetvalue(res, i, 7));
    }

    fclose(file);
    printf("Exported %d records to publication_history_export.csv\n", rows);

    PQclear(res);
    return VTL_res_kOk;
}