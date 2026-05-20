#include <VTL/history_administration/VTL_history_administration.h>
#include <VTL/history_administration/VTL_history_administration_data.h>
#include <VTL/user/VTL_user_data.h>
#include <VTL/utils/db/VTL_db.h>
#include <VTL/utils/VTL_time.h>
#include <VTL/utils/VTL_file.h>
#include <VTL/VTL_app_result.h>
#include <libpq-fe.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

// Глобальная БД
extern VTL_Database g_db;

// ==================== Вспомогательные ====================

static void print_publication_row(PGresult *res, int i)
{
    printf("  #%s | @%s | platform=%s | status=%s | time=%s",
           PQgetvalue(res, i, 0),
           PQgetvalue(res, i, 1),
           atoi(PQgetvalue(res, i, 3)) == VTL_PLATFORM_TELEGRAM ? "TG" : "VK",
           PQgetvalue(res, i, 4),
           PQgetvalue(res, i, 7));

    if (!PQgetisnull(res, i, 5))
    {
        printf(" | text=%s", PQgetvalue(res, i, 5));
    }
    if (!PQgetisnull(res, i, 6))
    {
        printf(" | media=%s", PQgetvalue(res, i, 6));
    }
    printf("\n");
}

static void time_to_sql(const VTL_Time *p_time, char *buf, size_t buf_size)
{
    time_t raw = (time_t)(*p_time);
    struct tm *tm_info = localtime(&raw);
    strftime(buf, buf_size, "%Y-%m-%d %H:%M:%S", tm_info);
}

static const char *SELECT_ALL =
    "SELECT id, user_nickname, publication_type, platform, "
    "status, text_file_name, media_file_name, published_at "
    "FROM publication_history ORDER BY published_at DESC";

// ==================== Создание таблицы ====================

bool VTL_user_history_db_CreateTable(VTL_Database *db)
{
    const char *sql =
        "CREATE TABLE IF NOT EXISTS publication_history ("
        "  id SERIAL PRIMARY KEY,"
        "  user_nickname VARCHAR(256) NOT NULL REFERENCES users(nickname),"
        "  publication_type INT NOT NULL,"
        "  platform INT NOT NULL,"
        "  status INT NOT NULL,"
        "  text_file_name VARCHAR(512),"
        "  media_file_name VARCHAR(512),"
        "  published_at TIMESTAMP NOT NULL DEFAULT NOW()"
        ")";

    return VTL_db_Execute(db, sql);
}

// ==================== Insert ====================

bool VTL_user_history_db_Insert(VTL_Database *db, VTL_UserHistory *history)
{
    char time_buf[64];
    time_to_sql(&history->publication_start_time, time_buf, sizeof(time_buf));

    const char *sql =
        "INSERT INTO publication_history "
        "(user_nickname, publication_type, platform, status, "
        "text_file_name, media_file_name, published_at) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7) RETURNING id";

    // Массив значений параметров
    const char *paramValues[7];
    paramValues[0] = history->user.nickname;
    paramValues[1] = NULL;
    paramValues[2] = NULL;
    paramValues[3] = NULL;
    paramValues[4] = history->has_text_file ? history->text_file_name : NULL;
    paramValues[5] = history->has_media_file ? history->media_file_name : NULL;
    paramValues[6] = time_buf;

    // Буферы для числовых значений
    char pub_type[20], platform[20], status[20];
    snprintf(pub_type, sizeof(pub_type), "%d", history->publication_type);
    snprintf(platform, sizeof(platform), "%d", history->platform);
    snprintf(status, sizeof(status), "%d", history->status);

    paramValues[1] = pub_type;
    paramValues[2] = platform;
    paramValues[3] = status;

    // Выполнение с параметрами
    PGresult *res = PQexecParams(db->conn,
                                 sql,
                                 7,           // количество параметров
                                 NULL,        // типы (или NULL)
                                 paramValues, // значения
                                 NULL,        // длины (NULL = строки)
                                 NULL,        // форматы (NULL = текст)
                                 0);

    PGresult *res = PQexec(db->conn, sql);

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        printf("Insert history failed: %s\n", PQerrorMessage(db->conn));
        PQclear(res);
        return false;
    }

    history->id = atoi(PQgetvalue(res, 0, 0));
    printf("Publication #%d saved (@%s)\n", history->id, history->user.nickname);

    PQclear(res);
    return true;
}

// ==================== ShowAll ====================

VTL_AppResult VTL_history_administration_ShowAll(void)
{
    PGresult *res = PQexec(g_db.conn, SELECT_ALL);

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        printf("Query failed: %s\n", PQerrorMessage(g_db.conn));
        PQclear(res);
        return VTL_res_kErr;
    }

    int rows = PQntuples(res);
    printf("\n=== All publications (%d) ===\n", rows);
    for (int i = 0; i < rows; i++)
    {
        print_publication_row(res, i);
    }

    PQclear(res);
    return VTL_res_kOk;
}

// ==================== DownloadAll ====================

VTL_AppResult VTL_history_administration_DownloadAll(void)
{
    PGresult *res = PQexec(g_db.conn, SELECT_ALL);

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        printf("Query failed: %s\n", PQerrorMessage(g_db.conn));
        PQclear(res);
        return VTL_res_kOk;
    }

    FILE *file = fopen("publication_history_export.csv", "w");
    if (!file)
    {
        printf("Cannot create export file\n");
        PQclear(res);
        return VTL_res_kErr;
    }

    // Заголовок CSV
    fprintf(file, "id,user_nickname,publication_type,platform,status,text_file,media_file,published_at\n");

    int rows = PQntuples(res);
    for (int i = 0; i < rows; i++)
    {
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

// ==================== CopyAll ====================

VTL_AppResult VTL_history_administration_CopyAll(void)
{
    const char *sql =
        "CREATE TABLE IF NOT EXISTS publication_history_backup AS "
        "SELECT * FROM publication_history";

    PGresult *res = PQexec(g_db.conn, sql);

    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        printf("Backup failed: %s\n", PQerrorMessage(g_db.conn));
        PQclear(res);
        return VTL_res_kErr;
    }

    printf("All publications copied to publication_history_backup\n");
    PQclear(res);
    return VTL_res_kOk;
}

// ==================== DeleteAll ====================

VTL_AppResult VTL_history_administration_DeleteAll(void)
{
    PGresult *res = PQexec(g_db.conn, "DELETE FROM publication_history");

    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        printf("Delete failed: %s\n", PQerrorMessage(g_db.conn));
        PQclear(res);
        return VTL_res_kErr;
    }

    printf("All publications deleted. Rows affected: %s\n", PQcmdTuples(res));
    PQclear(res);
    return VTL_res_kOk;
}

// ==================== DeleteBeforeTime ====================

VTL_AppResult VTL_history_administration_DeleteBeforeTime(const VTL_Time *p_time)
{
    char time_buf[64];
    time_to_sql(p_time, time_buf, sizeof(time_buf));

    const char *paramValues[1] = {time_buf};

    PGresult *res = PQexecParams(
        g_db.conn,
        "DELETE FROM publication_history WHERE published_at < $1",
        1,           // количество параметров
        NULL,        // типы (или NULL)
        paramValues, // значения
        NULL,        // длины (NULL = строки)
        NULL,        // форматы (NULL = текст)
        0);

    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        printf("Delete failed: %s\n", PQerrorMessage(g_db.conn));
        PQclear(res);
        return VTL_res_kErr;
    }

    printf("Deleted publications before %s. Rows: %s\n", time_buf, PQcmdTuples(res));
    PQclear(res);
    return VTL_res_kOk;
}

// ==================== DeleteAfterTime ====================

VTL_AppResult VTL_history_administration_DeleteAfterTime(const VTL_Time *p_time)
{
    char time_buf[64];
    time_to_sql(p_time, time_buf, sizeof(time_buf));

    const char *paramValues[1] = {time_buf};

    PGresult *res = PQexecParams(
        g_db.conn,
        "DELETE FROM publication_history WHERE published_at > $1",
        1,           // количество параметров
        NULL,        // типы (или NULL)
        paramValues, // значения
        NULL,        // длины (NULL = строки)
        NULL,        // форматы (NULL = текст)
        0);

    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        printf("Delete failed: %s\n", PQerrorMessage(g_db.conn));
        PQclear(res);
        return VTL_res_kErr;
    }

    printf("Deleted publications after %s. Rows: %s\n", time_buf, PQcmdTuples(res));
    PQclear(res);
    return VTL_res_kOk;
}

// ==================== ShowBeforeTime ====================

VTL_AppResult VTL_history_administration_ShowBeforeTime(const VTL_Time *p_time)
{
    char time_buf[64];
    time_to_sql(p_time, time_buf, sizeof(time_buf));

    const char *paramValues[1] = {time_buf};

    PGresult *res = PQexecParams(
        g_db.conn,
        "SELECT id, user_nickname, publication_type, platform, "
        "status, text_file_name, media_file_name, published_at "
        "FROM publication_history WHERE published_at < $1 "
        "ORDER BY published_at DESC",
        1,           // количество параметров
        NULL,        // типы (или NULL)
        paramValues, // значения
        NULL,        // длины (NULL = строки)
        NULL,        // форматы (NULL = текст)
        0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        printf("Query failed: %s\n", PQerrorMessage(g_db.conn));
        PQclear(res);
        return VTL_res_kErr;
    }

    int rows = PQntuples(res);
    printf("\n=== Publications before %s (%d) ===\n", time_buf, rows);
    for (int i = 0; i < rows; i++)
    {
        print_publication_row(res, i);
    }

    PQclear(res);
    return VTL_res_kOk;
}

// ==================== ShowAfterTime ====================

VTL_AppResult VTL_history_administration_ShowAfterTime(const VTL_Time *p_time)
{
    char time_buf[64];
    time_to_sql(p_time, time_buf, sizeof(time_buf));

    const char *paramValues[1] = {time_buf};

    PGresult *res = PQexecParams(
        g_db.conn,
        "SELECT id, user_nickname, publication_type, platform, "
        "status, text_file_name, media_file_name, published_at "
        "FROM publication_history WHERE published_at > $1 "
        "ORDER BY published_at DESC",
        1,           // количество параметров
        NULL,        // типы (или NULL)
        paramValues, // значения
        NULL,        // длины (NULL = строки)
        NULL,        // форматы (NULL = текст)
        0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        printf("Query failed: %s\n", PQerrorMessage(g_db.conn));
        PQclear(res);
        return VTL_res_kErr;
    }

    int rows = PQntuples(res);
    printf("\n=== Publications after %s (%d) ===\n", time_buf, rows);
    for (int i = 0; i < rows; i++)
    {
        print_publication_row(res, i);
    }

    PQclear(res);
    return VTL_res_kOk;
}

// ==================== ShowByUser ====================

VTL_AppResult VTL_history_administration_ShowByUser(const VTL_DecryptedUserName user_name)
{
    const char *paramValues[1] = {user_name};

    PGresult *res = PQexecParams(
        g_db.conn,
        "SELECT id, user_nickname, publication_type, platform, "
        "status, text_file_name, media_file_name, published_at "
        "FROM publication_history WHERE user_nickname = $1 "
        "ORDER BY published_at DESC",
        1,           // количество параметров
        NULL,        // типы (или NULL)
        paramValues, // значения
        NULL,        // длины (NULL = строки)
        NULL,        // форматы (NULL = текст)
        0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        printf("Query failed: %s\n", PQerrorMessage(g_db.conn));
        PQclear(res);
        return VTL_res_kErr;
    }

    int rows = PQntuples(res);
    printf("\n=== Publications by @%s (%d) ===\n", user_name, rows);
    for (int i = 0; i < rows; i++)
    {
        print_publication_row(res, i);
    }

    PQclear(res);
    return VTL_res_kOk;
}

// ==================== DeleteByUser ====================

VTL_AppResult VTL_history_administration_DeleteByUser(const VTL_DecryptedUserName user_name)
{
    const char *paramValues[1] = {user_name};

    PGresult *res = PQexecParams(
        g_db.conn,
        "DELETE FROM publication_history WHERE user_nickname = $1",
        1,           // количество параметров
        NULL,        // типы (или NULL)
        paramValues, // значения
        NULL,        // длины (NULL = строки)
        NULL,        // форматы (NULL = текст)
        0);

    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        printf("Delete failed: %s\n", PQerrorMessage(g_db.conn));
        PQclear(res);
        return VTL_res_kErr;
    }

    printf("Deleted publications by @%s. Rows: %s\n", user_name, PQcmdTuples(res));
    PQclear(res);
    return VTL_res_kOk;
}