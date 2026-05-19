#include <VTL/history_administration/db/VTL_history_administration_edit.h>
#include <libpq-fe.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static void time_to_sql(const VTL_Time* p_time, char* buf, size_t buf_size) {
    time_t raw = (time_t)(*p_time);
    struct tm* tm_info = localtime(&raw);
    strftime(buf, buf_size, "%Y-%m-%d %H:%M:%S", tm_info);
}

VTL_AppResult VTL_history_administration_UpdateStatus(
        VTL_Database* db,
        int publication_id,
        VTL_PublicationStatus new_status) {

    char sql[512];
    snprintf(sql, sizeof(sql),
        "UPDATE publication_history SET status = %d, updated_at = NOW() WHERE id = %d",
        new_status, publication_id);

    PGresult* res = PQexec(db->conn, sql);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        printf("Update status failed: %s\n", PQerrorMessage(db->conn));
        PQclear(res);
        return VTL_res_kErr;
    }

    if (atoi(PQcmdTuples(res)) == 0) {
        printf("Publication #%d not found.\n", publication_id);
        PQclear(res);
        return VTL_res_kErr;
    }

    printf("Publication #%d status updated to %d.\n", publication_id, new_status);
    PQclear(res);
    return VTL_res_kOk;
}

VTL_AppResult VTL_history_administration_UpdateTextFile(
        VTL_Database* db,
        int publication_id,
        const char* new_text_file) {

    char sql[512];
    snprintf(sql, sizeof(sql),
        "UPDATE publication_history SET text_file_name = '%s', updated_at = NOW() WHERE id = %d",
        new_text_file, publication_id);

    PGresult* res = PQexec(db->conn, sql);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        printf("Update text file failed: %s\n", PQerrorMessage(db->conn));
        PQclear(res);
        return VTL_res_kErr;
    }

    printf("Publication #%d text file updated to '%s'.\n", publication_id, new_text_file);
    PQclear(res);
    return VTL_res_kOk;
}

VTL_AppResult VTL_history_administration_UpdateMediaFile(
        VTL_Database* db,
        int publication_id,
        const char* new_media_file) {

    char sql[512];
    snprintf(sql, sizeof(sql),
        "UPDATE publication_history SET media_file_name = '%s', updated_at = NOW() WHERE id = %d",
        new_media_file, publication_id);

    PGresult* res = PQexec(db->conn, sql);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        printf("Update media file failed: %s\n", PQerrorMessage(db->conn));
        PQclear(res);
        return VTL_res_kErr;
    }

    printf("Publication #%d media file updated to '%s'.\n", publication_id, new_media_file);
    PQclear(res);
    return VTL_res_kOk;
}

VTL_AppResult VTL_history_administration_UpdatePlatform(
        VTL_Database* db,
        int publication_id,
        VTL_Platform new_platform) {

    char sql[512];
    snprintf(sql, sizeof(sql),
        "UPDATE publication_history SET platform = %d, updated_at = NOW() WHERE id = %d",
        new_platform, publication_id);

    PGresult* res = PQexec(db->conn, sql);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        printf("Update platform failed: %s\n", PQerrorMessage(db->conn));
        PQclear(res);
        return VTL_res_kErr;
    }

    printf("Publication #%d platform updated.\n", publication_id);
    PQclear(res);
    return VTL_res_kOk;
}

VTL_AppResult VTL_history_administration_Reschedule(
        VTL_Database* db,
        int publication_id,
        VTL_Time new_time) {

    char time_buf[64];
    time_to_sql(&new_time, time_buf, sizeof(time_buf));

    char sql[512];
    snprintf(sql, sizeof(sql),
        "UPDATE publication_history "
        "SET published_at = '%s', status = %d, updated_at = NOW() "
        "WHERE id = %d",
        time_buf, VTL_PUBLICATION_SCHEDULED, publication_id);

    PGresult* res = PQexec(db->conn, sql);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        printf("Reschedule failed: %s\n", PQerrorMessage(db->conn));
        PQclear(res);
        return VTL_res_kErr;
    }

    printf("Publication #%d rescheduled to %s.\n", publication_id, time_buf);
    PQclear(res);
    return VTL_res_kOk;
}