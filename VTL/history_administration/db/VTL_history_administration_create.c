#include <VTL/history_administration/db/VTL_history_administration_create.h>
#include <libpq-fe.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <VTL/history_administration/VTL_history_administration_data.h>
#include <VTL/utils/db/VTL_db.h>
#include <VTL/VTL_app_result.h>
#include <VTL/utils/VTL_time_utils.h>

VTL_AppResult VTL_history_administration_CreateTable(VTL_Database* db) {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS publication_history ("
        "  id SERIAL PRIMARY KEY,"
        "  user_nickname VARCHAR(256) NOT NULL REFERENCES users(nickname),"
        "  publication_type INT NOT NULL,"
        "  platform INT NOT NULL,"
        "  status INT NOT NULL,"
        "  text_file_name VARCHAR(512),"
        "  media_file_name VARCHAR(512),"
        "  published_at TIMESTAMP NOT NULL DEFAULT NOW(),"
        "  updated_at TIMESTAMP,"
        "  description TEXT"
        ")";

    if (!VTL_db_Execute(db, sql)) {
        return VTL_res_kErr;
    }

    printf("Table publication_history created.\n");
    return VTL_res_kOk;
}

VTL_AppResult VTL_history_administration_Insert(VTL_Database* db, VTL_UserHistory* history) {
    char time_buf[64];
    VTL_time_to_sql(&history->publication_start_time, time_buf, sizeof(time_buf));

    char sql[2048];
    snprintf(sql, sizeof(sql),
        "INSERT INTO publication_history "
        "(user_nickname, publication_type, platform, status, "
        "text_file_name, media_file_name, published_at) "
        "VALUES ('%s', %d, %d, %d, %s%s%s, %s%s%s, '%s') RETURNING id",
        history->user.nickname,
        history->publication_type,
        history->platform,
        history->status,
        history->has_text_file  ? "'" : "",
        history->has_text_file  ? history->text_file_name : "NULL",
        history->has_text_file  ? "'" : "",
        history->has_media_file ? "'" : "",
        history->has_media_file ? history->media_file_name : "NULL",
        history->has_media_file ? "'" : "",
        time_buf
    );

    PGresult* res = PQexec(db->conn, sql);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        printf("Insert failed: %s\n", PQerrorMessage(db->conn));
        PQclear(res);
        return VTL_res_kErr;
    }

    history->id = atoi(PQgetvalue(res, 0, 0));
    printf("Publication #%d inserted (@%s)\n", history->id, history->user.nickname);

    PQclear(res);
    return VTL_res_kOk;
}

VTL_AppResult VTL_history_administration_InsertScheduled(
        VTL_Database* db,
        VTL_UserHistory* history,
        VTL_Time scheduled_time) {

    history->publication_start_time = scheduled_time;
    history->status = VTL_PUBLICATION_SCHEDULED;

    return VTL_history_administration_Insert(db, history);
}

VTL_AppResult VTL_history_administration_CopyAll(VTL_Database* db) {
    // Удаляем старый бэкап если есть
    VTL_db_Execute(db, "DROP TABLE IF EXISTS publication_history_backup");

    const char* sql =
        "CREATE TABLE publication_history_backup AS "
        "SELECT * FROM publication_history";

    PGresult* res = PQexec(db->conn, sql);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        printf("Backup failed: %s\n", PQerrorMessage(db->conn));
        PQclear(res);
        return VTL_res_kErr;
    }

    printf("All publications backed up to publication_history_backup.\n");
    PQclear(res);
    return VTL_res_kOk;
}