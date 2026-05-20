#include <VTL/history_administration/db/VTL_history_administration_delete.h>
#include <libpq-fe.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <VTL/utils/VTL_time_utils.h>

static VTL_AppResult execute_delete(VTL_Database* db, const char* sql, const char* description) {
    PGresult* res = PQexec(db->conn, sql);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        printf("Delete failed: %s\n", PQerrorMessage(db->conn));
        PQclear(res);
        return VTL_res_kErr;
    }

    printf("%s. Rows deleted: %s\n", description, PQcmdTuples(res));
    PQclear(res);
    return VTL_res_kOk;
}

VTL_AppResult VTL_history_administration_DeleteAll(VTL_Database* db) {
    return execute_delete(db,
        "DELETE FROM publication_history",
        "All publications deleted");
}

VTL_AppResult VTL_history_administration_DeleteByUser(VTL_Database* db, const char* user_name) {
    char sql[512];
    snprintf(sql, sizeof(sql),
        "DELETE FROM publication_history WHERE user_nickname = '%s'",
        user_name);

    char desc[256];
    snprintf(desc, sizeof(desc), "Publications by @%s deleted", user_name);

    return execute_delete(db, sql, desc);
}

VTL_AppResult VTL_history_administration_DeleteById(VTL_Database* db, int publication_id) {
    char sql[256];
    snprintf(sql, sizeof(sql),
        "DELETE FROM publication_history WHERE id = %d",
        publication_id);

    char desc[128];
    snprintf(desc, sizeof(desc), "Publication #%d deleted", publication_id);

    return execute_delete(db, sql, desc);
}

VTL_AppResult VTL_history_administration_DeleteBeforeTime(VTL_Database* db, const VTL_Time* p_time) {
    char time_buf[64];
    VTL_time_to_sql(p_time, time_buf, sizeof(time_buf));

    char sql[512];
    snprintf(sql, sizeof(sql),
        "DELETE FROM publication_history WHERE published_at < '%s'",
        time_buf);

    char desc[256];
    snprintf(desc, sizeof(desc), "Publications before %s deleted", time_buf);

    return execute_delete(db, sql, desc);
}

VTL_AppResult VTL_history_administration_DeleteAfterTime(VTL_Database* db, const VTL_Time* p_time) {
    char time_buf[64];
    VTL_time_to_sql(p_time, time_buf, sizeof(time_buf));

    char sql[512];
    snprintf(sql, sizeof(sql),
        "DELETE FROM publication_history WHERE published_at > '%s'",
        time_buf);

    char desc[256];
    snprintf(desc, sizeof(desc), "Publications after %s deleted", time_buf);

    return execute_delete(db, sql, desc);
}