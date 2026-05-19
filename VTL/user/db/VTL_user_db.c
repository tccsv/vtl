#include <VTL/user/db/VTL_user_db.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

bool VTL_user_db_CreateTable(VTL_Database* db) {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "  id SERIAL PRIMARY KEY,"
        "  first_name VARCHAR(256) NOT NULL,"
        "  last_name VARCHAR(256),"
        "  email_encrypted VARCHAR(512) NOT NULL,"
        "  nickname VARCHAR(256) NOT NULL UNIQUE"
        ")";

    return VTL_db_Execute(db, sql);
}

bool VTL_user_db_Insert(VTL_Database* db, VTL_User* user) {
    // Email хранится зашифрованным в HEX
    // Конвертируем зашифрованные байты в HEX для хранения в БД
    char email_hex[VTL_ENCRYPTED_STRING_SIZE * 2 + 1];
    size_t email_len = strlen(user->email);
    for (size_t i = 0; i < email_len; i++) {
        sprintf(&email_hex[i * 2], "%02x", (unsigned char)user->email[i]);
    }
    email_hex[email_len * 2] = '\0';

    char sql[2048];
    if (user->has_last_name) {
        snprintf(sql, sizeof(sql),
            "INSERT INTO users (first_name, last_name, email_encrypted, nickname) "
            "VALUES ('%s', '%s', '%s', '%s') RETURNING id",
            user->first_name, user->last_name, email_hex, user->nickname);
    } else {
        snprintf(sql, sizeof(sql),
            "INSERT INTO users (first_name, email_encrypted, nickname) "
            "VALUES ('%s', '%s', '%s') RETURNING id",
            user->first_name, email_hex, user->nickname);
    }

    PGresult* res = PQexec(db->conn, sql);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        printf("Insert failed: %s\n", PQerrorMessage(db->conn));
        PQclear(res);
        return false;
    }

    // Получаем ID
    user->id = atoi(PQgetvalue(res, 0, 0));
    printf("User '%s' inserted with id=%d\n", user->nickname, user->id);

    PQclear(res);
    return true;
}

bool VTL_user_db_FindByNickname(VTL_Database* db, const char* nickname, VTL_User* user) {
    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT id, first_name, last_name, email_encrypted, nickname "
        "FROM users WHERE nickname = '%s'", nickname);

    PGresult* res = PQexec(db->conn, sql);

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        printf("User '%s' not found.\n", nickname);
        PQclear(res);
        return false;
    }

    VTL_user_Init(user);

    user->id = atoi(PQgetvalue(res, 0, 0));
    VTL_user_SetFirstName(user, PQgetvalue(res, 0, 1));

    char* last_name = PQgetvalue(res, 0, 2);
    if (!PQgetisnull(res, 0, 2)) {
        VTL_user_SetLastName(user, last_name);
    }

    // Расшифровка email из HEX
    char* email_hex = PQgetvalue(res, 0, 3);
    size_t hex_len = strlen(email_hex);
    VTL_EncryptedString encrypted_email;
    for (size_t i = 0; i < hex_len / 2; i++) {
        unsigned int byte;
        sscanf(&email_hex[i * 2], "%02x", &byte);
        encrypted_email[i] = (char)byte;
    }
    encrypted_email[hex_len / 2] = '\0';
    memcpy(user->email, encrypted_email, sizeof(encrypted_email));

    snprintf(user->nickname, sizeof(user->nickname), "%s", PQgetvalue(res, 0, 4));

    PQclear(res);
    return true;
}

bool VTL_user_db_FindAll(VTL_Database* db) {
    PGresult* res = PQexec(db->conn, "SELECT id, first_name, last_name, nickname FROM users");

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        printf("Query failed: %s\n", PQerrorMessage(db->conn));
        PQclear(res);
        return false;
    }

    int rows = PQntuples(res);
    printf("\n=== All users (%d) ===\n", rows);
    for (int i = 0; i < rows; i++) {
        printf("  #%s | %s %s | @%s\n",
            PQgetvalue(res, i, 0),
            PQgetvalue(res, i, 1),
            PQgetisnull(res, i, 2) ? "" : PQgetvalue(res, i, 2),
            PQgetvalue(res, i, 3));
    }

    PQclear(res);
    return true;
}

bool VTL_user_db_DeleteByNickname(VTL_Database* db, const char* nickname) {
    char sql[512];
    snprintf(sql, sizeof(sql), "DELETE FROM users WHERE nickname = '%s'", nickname);
    return VTL_db_Execute(db, sql);
}