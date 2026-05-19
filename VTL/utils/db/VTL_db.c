#include <VTL/utils/db/VTL_db.h>
#include <stdio.h>

bool VTL_db_Connect(VTL_Database* db, const char* conn_string) {
    db->conn = PQconnectdb(conn_string);

    if (PQstatus(db->conn) != CONNECTION_OK) {
        printf("DB connection failed: %s\n", PQerrorMessage(db->conn));
        PQfinish(db->conn);
        db->conn = NULL;
        return false;
    }

    printf("Connected to database!\n");
    return true;
}

void VTL_db_Disconnect(VTL_Database* db) {
    if (db->conn != NULL) {
        PQfinish(db->conn);
        db->conn = NULL;
        printf("Disconnected from database.\n");
    }
}

bool VTL_db_Execute(VTL_Database* db, const char* sql) {
    PGresult* res = PQexec(db->conn, sql);
    ExecStatusType status = PQresultStatus(res);

    if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK) {
        printf("SQL error: %s\n", PQerrorMessage(db->conn));
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}