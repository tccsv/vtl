#include <VTL/user/db/VTL_user_db_error.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

bool VTL_user_db_CreateErrorTable(VTL_Database *db)
{
    const char *sql =
        "CREATE TABLE IF NOT EXISTS user_errors ("
        "  nickname VARCHAR(256) PRIMARY KEY,"
        "  error_time TIMESTAMP NOT NULL DEFAULT NOW(),"
        "  error_message VARCHAR(512) NOT NULL"
        ")";

    return VTL_db_Execute(db, sql);
}

bool VTL_user_db_Insert(VTL_Database *db, VTL_Error *error)
{
    char time_buf[64];
    VTL_time_to_sql(&error->error_time, time_buf, sizeof(time_buf));

    char sql[2048];

    snprintf(sql, sizeof(sql),
             "INSERT INTO user_errors (nickname, error_time, error_message) "
             "VALUES ('%s', '%s', '%s') RETURNING nickname",
             error->nickname, time_buf, error->error_message);

    PGresult *res = PQexec(db->conn, sql);

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        printf("Insert failed: %s\n", PQerrorMessage(db->conn));
        PQclear(res);
        return false;
    }

    printf("User error '%s' inserted with nickname=%d\n", error->error_message, error->nickname);

    PQclear(res);
    return true;
}

bool VTL_user_db_FindErrorByUserNickname(VTL_Database *db, const char *nickname, VTL_Error* error)
{
    const char *paramValues[1];
    paramValues[0] = nickname;

    PGresult *res = PQexecParams(db->conn,
                                 "SELECT error_time, error_message "
                                 "FROM users WHERE nickname = $1",
                                 1,           // количество параметров
                                 NULL,        // OID типов
                                 paramValues, // значения параметров
                                 NULL,        // длины
                                 NULL,        // форматы
                                 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
    {
        printf("User '%s' not found.\n", nickname);
        PQclear(res);
        return false;
    }

    VTL_error_Init(error);

    VTL_user_SetNickname(error, nickname);
    VTL_user_SetErrorMessage(error, PQgetvalue(res, 0, 2));

    printf("User error (@%s) saved (@%s)\n", error->nickname, error->error_message);

    PQclear(res);
    return true;
}

bool VTL_user_db_FindAll(VTL_Database *db)
{
    PGresult *res = PQexec(db->conn, "SELECT nickname, error_time, error_message FROM user_errors");

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        printf("Query failed: %s\n", PQerrorMessage(db->conn));
        PQclear(res);
        return false;
    }

    int rows = PQntuples(res);
    printf("\n=== All user errors (%d) ===\n", rows);
    for (int i = 0; i < rows; i++)
    {
        printf("  #%s | %s %s | @%s\n",
               PQgetvalue(res, i, 0),
               PQgetvalue(res, i, 1),
               PQgetvalue(res, i, 2));
    }

    PQclear(res);
    return true;
}

bool VTL_user_db_DeleteByNickname(VTL_Database *db, const char *nickname)
{
    const char *paramValues[1];
    paramValues[0] = nickname;

    PGresult *res = PQexecParams(db->conn, "DELETE FROM user_errors WHERE nickname = $1",
                                 1,           // количество параметров
                                 NULL,        // OID типов
                                 paramValues, // значения параметров
                                 NULL,        // длины
                                 NULL,        // форматы
                                 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
    {
        printf("User error '%s' not found.\n", nickname);
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}