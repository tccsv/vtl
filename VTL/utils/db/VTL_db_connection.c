#include <VTL/utils/db/VTL_db_connection.h>

PGconn* VTL_connect_to_db(VTL_db_Credentals* p_credentals){
    char connurl[1024];
    snprintf(connurl, sizeof(connurl),
    "dbname='%s' user='%s' password='%s' hostaddr='%s' port='%s'",
    p_credentals->dbname, p_credentals->user, p_credentals->password, p_credentals->host, p_credentals->port);
    PGconn *conn = PQconnectdb(connurl);

    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
    }
    return conn;
}