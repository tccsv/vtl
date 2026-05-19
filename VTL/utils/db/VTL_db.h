#ifndef _VTL_DB_H
#define _VTL_DB_H
#ifdef __cplusplus
extern "C" {
#endif

#include <libpq-fe.h>
#include <stdbool.h>

typedef struct _VTL_Database {
    PGconn* conn;
} VTL_Database;

bool VTL_db_Connect(VTL_Database* db, const char* conn_string);
void VTL_db_Disconnect(VTL_Database* db);
bool VTL_db_Execute(VTL_Database* db, const char* sql);

#ifdef __cplusplus
}
#endif
#endif