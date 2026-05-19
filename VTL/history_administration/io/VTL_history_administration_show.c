#include <VTL/history_administration/io/VTL_history_administration_show.h>
#include <VTL/history_administration/db/VTL_history_administration.h>
#include <VTL/utils/db/VTL_db.h>
#include <libpq-fe.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

// ==================== Вспомогательные ====================

static const char* platform_to_str(VTL_Platform platform) {
    switch (platform) {
        case VTL_PLATFORM_TELEGRAM: return "Telegram";
        case VTL_PLATFORM_VK:       return "VK";
        default:                    return "Unknown";
    }
}

static const char* status_to_str(VTL_PublicationStatus status) {
    switch (status) {
        case VTL_PUBLICATION_DRAFT:     return "Draft";
        case VTL_PUBLICATION_SCHEDULED: return "Scheduled";
        case VTL_PUBLICATION_PUBLISHED: return "Published";
        case VTL_PUBLICATION_FAILED:    return "Failed";
        default:                        return "Unknown";
    }
}

static const char* type_to_str(VTL_PublicationType type) {
    switch (type) {
        case VTL_PUBLICATION_TEXT_ONLY:      return "Text";
        case VTL_PUBLICATION_MEDIA_ONLY:     return "Media";
        case VTL_PUBLICATION_MEDIA_WITH_TEXT: return "Media+Text";
        default:                             return "Unknown";
    }
}

static const char* status_to_icon(VTL_PublicationStatus status) {
    switch (status) {
        case VTL_PUBLICATION_DRAFT:     return "[ ]";
        case VTL_PUBLICATION_SCHEDULED: return "[~]";
        case VTL_PUBLICATION_PUBLISHED: return "[+]";
        case VTL_PUBLICATION_FAILED:    return "[!]";
        default:                        return "[?]";
    }
}

static void format_time(VTL_Time t, char* buf, size_t buf_size) {
    time_t raw = (time_t)t;
    struct tm* tm_info = localtime(&raw);
    strftime(buf, buf_size, "%Y-%m-%d %H:%M:%S", tm_info);
}

// ==================== Форматирование ====================

void VTL_show_Separator(void) {
    printf("+---------+------------------+------------+----------+------------+---------------------+\n");
}

void VTL_show_TableHeader(void) {
    VTL_show_Separator();
    printf("| %-7s | %-16s | %-10s | %-8s | %-10s | %-19s |\n",
           "ID", "User", "Platform", "Status", "Type", "Published at");
    VTL_show_Separator();
}

void VTL_show_PublicationShort(const VTL_UserHistory* history) {
    char time_buf[64];
    format_time(history->publication_start_time, time_buf, sizeof(time_buf));

    printf("| %-7d | @%-15s | %-10s | %-3s %-4s | %-10s | %-19s |\n",
           history->id,
           history->user.nickname,
           platform_to_str(history->platform),
           status_to_icon(history->status),
           "",
           type_to_str(history->publication_type),
           time_buf);
}

void VTL_show_PublicationDetailed(const VTL_UserHistory* history) {
    char time_buf[64];
    format_time(history->publication_start_time, time_buf, sizeof(time_buf));

    printf("\n");
    printf("  ┌─────────────────────────────────────────┐\n");
    printf("  │ Publication #%-4d                        │\n", history->id);
    printf("  ├─────────────────────────────────────────┤\n");
    printf("  │ User:       @%-26s │\n", history->user.nickname);
    printf("  │ Platform:   %-27s │\n", platform_to_str(history->platform));
    printf("  │ Type:       %-27s │\n", type_to_str(history->publication_type));
    printf("  │ Status:     %s %-23s │\n", status_to_icon(history->status),
           status_to_str(history->status));
    printf("  │ Time:       %-27s │\n", time_buf);

    if (history->has_text_file) {
        printf("  │ Text file:  %-27s │\n", history->text_file_name);
    } else {
        printf("  │ Text file:  %-27s │\n", "(none)");
    }

    if (history->has_media_file) {
        printf("  │ Media file: %-27s │\n", history->media_file_name);
    } else {
        printf("  │ Media file: %-27s │\n", "(none)");
    }

    printf("  └─────────────────────────────────────────┘\n");
}

// ==================== Статистика ====================

VTL_AppResult VTL_show_Statistics(VTL_Database* db) {
    const char* sql =
        "SELECT "
        "  COUNT(*) as total, "
        "  COUNT(*) FILTER (WHERE platform = 1) as telegram_count, "
        "  COUNT(*) FILTER (WHERE platform = 2) as vk_count, "
        "  COUNT(*) FILTER (WHERE status = 0) as draft_count, "
        "  COUNT(*) FILTER (WHERE status = 1) as scheduled_count, "
        "  COUNT(*) FILTER (WHERE status = 2) as published_count, "
        "  COUNT(*) FILTER (WHERE status = 3) as failed_count, "
        "  COUNT(*) FILTER (WHERE text_file_name IS NOT NULL AND media_file_name IS NOT NULL) as media_text_count, "
        "  COUNT(*) FILTER (WHERE text_file_name IS NULL AND media_file_name IS NOT NULL) as media_only_count, "
        "  COUNT(*) FILTER (WHERE text_file_name IS NOT NULL AND media_file_name IS NULL) as text_only_count, "
        "  COUNT(DISTINCT user_nickname) as unique_users "
        "FROM publication_history";

    PGresult* res = PQexec(db->conn, sql);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        printf("Statistics query failed: %s\n", PQerrorMessage(db->conn));
        PQclear(res);
        return VTL_res_kErr;
    }

    int total     = atoi(PQgetvalue(res, 0, 0));
    int tg        = atoi(PQgetvalue(res, 0, 1));
    int vk        = atoi(PQgetvalue(res, 0, 2));
    int draft     = atoi(PQgetvalue(res, 0, 3));
    int scheduled = atoi(PQgetvalue(res, 0, 4));
    int published = atoi(PQgetvalue(res, 0, 5));
    int failed    = atoi(PQgetvalue(res, 0, 6));
    int mt        = atoi(PQgetvalue(res, 0, 7));
    int mo        = atoi(PQgetvalue(res, 0, 8));
    int to        = atoi(PQgetvalue(res, 0, 9));
    int users     = atoi(PQgetvalue(res, 0, 10));

    printf("\n");
    printf("  ╔═══════════════════════════════════════════╗\n");
    printf("  ║         Publication Statistics             ║\n");
    printf("  ╠═══════════════════════════════════════════╣\n");
    printf("  ║  Total publications:  %-6d               ║\n", total);
    printf("  ║  Unique users:        %-6d               ║\n", users);
    printf("  ╠═══════════════════════════════════════════╣\n");
    printf("  ║  By Platform:                             ║\n");
    printf("  ║    Telegram:          %-6d               ║\n", tg);
    printf("  ║    VK:                %-6d               ║\n", vk);
    printf("  ╠═══════════════════════════════════════════╣\n");
    printf("  ║  By Status:                               ║\n");
    printf("  ║    [ ] Draft:         %-6d               ║\n", draft);
    printf("  ║    [~] Scheduled:     %-6d               ║\n", scheduled);
    printf("  ║    [+] Published:     %-6d               ║\n", published);
    printf("  ║    [!] Failed:        %-6d               ║\n", failed);
    printf("  ╠═══════════════════════════════════════════╣\n");
    printf("  ║  By Type:                                 ║\n");
    printf("  ║    Text only:         %-6d               ║\n", to);
    printf("  ║    Media only:        %-6d               ║\n", mo);
    printf("  ║    Media + Text:      %-6d               ║\n", mt);
    printf("  ╚═══════════════════════════════════════════╝\n");

    PQclear(res);
    return VTL_res_kOk;
}

// ==================== Интерактивный просмотр по пользователю ====================

VTL_AppResult VTL_show_UserHistoryInteractive(VTL_Database* db) {
    char nickname[256];
    printf("\nEnter username: @");
    if (scanf("%255s", nickname) != 1) {
        printf("Invalid input.\n");
        return VTL_res_kErr;
    }

    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT id, user_nickname, publication_type, platform, "
        "status, text_file_name, media_file_name, published_at "
        "FROM publication_history WHERE user_nickname = '%s' "
        "ORDER BY published_at DESC",
        nickname);

    PGresult* res = PQexec(db->conn, sql);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        printf("Query failed: %s\n", PQerrorMessage(db->conn));
        PQclear(res);
        return VTL_res_kErr;
    }

    int rows = PQntuples(res);
    if (rows == 0) {
        printf("No publications found for @%s\n", nickname);
        PQclear(res);
        return VTL_res_kOk;
    }

    printf("\nFound %d publications for @%s\n", rows, nickname);

    for (int i = 0; i < rows; i++) {
        VTL_UserHistory h;
        memset(&h, 0, sizeof(h));

        h.id = atoi(PQgetvalue(res, i, 0));
        snprintf(h.user.nickname, sizeof(h.user.nickname), "%s", PQgetvalue(res, i, 1));
        h.publication_type = atoi(PQgetvalue(res, i, 2));
        h.platform = atoi(PQgetvalue(res, i, 3));
        h.status = atoi(PQgetvalue(res, i, 4));

        if (!PQgetisnull(res, i, 5)) {
            snprintf(h.text_file_name, sizeof(h.text_file_name), "%s", PQgetvalue(res, i, 5));
            h.has_text_file = true;
        }
        if (!PQgetisnull(res, i, 6)) {
            snprintf(h.media_file_name, sizeof(h.media_file_name), "%s", PQgetvalue(res, i, 6));
            h.has_media_file = true;
        }

        // Парсим timestamp из строки
        struct tm tm_info;
        memset(&tm_info, 0, sizeof(tm_info));
        strptime(PQgetvalue(res, i, 7), "%Y-%m-%d %H:%M:%S", &tm_info);
        h.publication_start_time = (VTL_Time)mktime(&tm_info);

        VTL_show_PublicationDetailed(&h);
    }

    PQclear(res);
    return VTL_res_kOk;
}

// ==================== Меню администратора ====================

VTL_AppResult VTL_show_AdminMenu(VTL_Database* db) {
    int choice = 0;

    while (choice != 9) {
        printf("\n");
        printf("  ┌───────────────────────────────────┐\n");
        printf("  │     Publication Admin Menu         │\n");
        printf("  ├───────────────────────────────────┤\n");
        printf("  │  1. Show all publications         │\n");
        printf("  │  2. Show by user                  │\n");
        printf("  │  3. Show statistics               │\n");
        printf("  │  4. Export to CSV                 │\n");
        printf("  │  5. Delete by user                │\n");
        printf("  │  6. Delete all                    │\n");
        printf("  │  7. Backup all                    │\n");
        printf("  │  8. Find by ID                    │\n");
        printf("  │  9. Exit                          │\n");
        printf("  └───────────────────────────────────┘\n");
        printf("  Choice: ");

        if (scanf("%d", &choice) != 1) {
            printf("Invalid input.\n");
            // Очистить буфер
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            continue;
        }

        switch (choice) {
            case 1:
                VTL_history_administration_ShowAll(db);
                break;

            case 2:
                VTL_show_UserHistoryInteractive(db);
                break;

            case 3:
                VTL_show_Statistics(db);
                break;

            case 4:
                VTL_history_administration_DownloadAll(db);
                break;

            case 5: {
                char nickname[256];
                printf("Enter username to delete: @");
                scanf("%255s", nickname);
                VTL_history_administration_DeleteByUser(db, nickname);
                break;
            }

            case 6: {
                char confirm[4];
                printf("Are you sure? (yes/no): ");
                scanf("%3s", confirm);
                if (strcmp(confirm, "yes") == 0) {
                    VTL_history_administration_DeleteAll(db);
                } else {
                    printf("Cancelled.\n");
                }
                break;
            }

            case 7:
                VTL_history_administration_CopyAll(db);
                break;

            case 8: {
                int id;
                printf("Enter publication ID: ");
                scanf("%d", &id);
                VTL_UserHistory found;
                if (VTL_history_administration_FindById(db, id, &found) == VTL_res_kOk) {
                    VTL_show_PublicationDetailed(&found);
                }
                break;
            }

            case 9:
                printf("Bye!\n");
                break;

            default:
                printf("Unknown option.\n");
                break;
        }
    }

    return VTL_res_kOk;
}