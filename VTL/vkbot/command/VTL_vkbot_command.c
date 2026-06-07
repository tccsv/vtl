/* localtime_r — POSIX; при -std=c11 объявление скрыто, явно просим POSIX. */
#if !defined(_WIN32)
#  define _POSIX_C_SOURCE 200809L
#endif

#include <VTL/vkbot/command/VTL_vkbot_command.h>
#include <VTL/vkbot/api/VTL_vkbot_api.h>
#include <VTL/vkbot/VTL_vkbot_data.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char* VTL_vkbot_command_kHelp =
    "Я бот для учёта личных финансов.\n"
    "Команды можно писать со слешем или без него.\n"
    "\n"
    "Операции:\n"
    "/spend <сумма> <описание> — записать расход\n"
    "/income <сумма> <описание> — записать доход\n"
    "/balance — текущий баланс\n"
    "/list — все операции\n"
    "/expenses — только расходы\n"
    "/incomes — только доходы\n"
    "/edit <N> <сумма> — изменить сумму\n"
    "/desc <N> <текст> — изменить описание\n"
    "/del <N> — удалить операцию\n"
    "/clear — удалить все операции\n"
    "/find <текст> — поиск по описанию\n"
    "/first — первая операция\n"
    "/last — последняя операция\n"
    "/count — число операций\n"
    "/stats — сводка\n"
    "/export — выгрузить списком\n"
    "/today — операции за сегодня\n"
    "/max — крупнейший расход\n"
    "/avg — средний расход\n"
    "\n"
    "Категории и лимит:\n"
    "/setcat <N> <категория> — категория операции\n"
    "/bycat <категория> — операции категории\n"
    "/addcat <название> — добавить категорию\n"
    "/cats — список категорий\n"
    "/delcat <N> — удалить категорию\n"
    "/budget <сумма> — задать лимит расходов\n"
    "/left — остаток лимита\n"
    "\n"
    "Сервис:\n"
    "/id — мой peer_id\n"
    "/time — время сервера\n"
    "/ping — проверка связи\n"
    "/help — эта справка";

/* ============================ помощники ============================ */

/* Пропускает ведущие пробелы. */
static const char* VTL_vkbot_command_Skip(const char* s)
{
    while (*s && isspace((unsigned char)*s)) ++s;
    return s;
}

/* Копирует первый токен (до пробела) из s в tok; возвращает остаток строки. */
static const char* VTL_vkbot_command_NextToken(const char* s, char* tok, size_t cap)
{
    s = VTL_vkbot_command_Skip(s);
    size_t n = 0;
    while (s[n] && !isspace((unsigned char)s[n]) && n + 1 < cap) {
        tok[n] = s[n];
        ++n;
    }
    tok[n] = '\0';
    return VTL_vkbot_command_Skip(s + n);
}

/* Форматирует момент времени как "дд.мм ЧЧ:ММ". */
static void VTL_vkbot_command_FormatTime(time_t t, char* out, size_t cap)
{
    struct tm tm_t;
#if defined(_WIN32)
    localtime_s(&tm_t, &t);
#else
    localtime_r(&t, &tm_t);
#endif
    strftime(out, cap, "%d.%m %H:%M", &tm_t);
}

/* Разбирает положительную сумму из строки. 1 — ок (через out), 0 — ошибка. */
static int VTL_vkbot_command_ParseAmount(const char* s, double* out)
{
    if (!s || !*s || !out) return 0;
    char* end = NULL;
    double v = strtod(s, &end);
    if (end == s || v <= 0) return 0;
    *out = v;
    return 1;
}

/* Отправляет text в диалог; ошибки транспорта только логируются. */
static void VTL_vkbot_command_Reply(const char* token, const char* peer_id,
                                    const char* text)
{
    if (VTL_vkbot_api_SendMessage(token, peer_id, text) != VTL_res_kOk)
        fprintf(stderr, "[vkbot] messages.send в %s не удался\n", peer_id);
}

/* ===================== обработчики команд ===========================
 * Каждый метод отвечает ровно за одну команду. Единая сигнатура —
 * чтобы разложить их в таблицу-диспетчер ниже. arg — аргументы после
 * имени команды (без ведущих пробелов). Нумерация [N] — функционал.
 * =================================================================== */

/* [1] /spend <сумма> <описание> — записать расход. */
static void VTL_vkbot_command_HandleSpend(VTL_vkbot_Store* store, const char* token,
                                          const char* peer, const char* arg)
{
    char reply[VTL_VKBOT_REPLY_MAX];
    char num[32];
    const char* desc = VTL_vkbot_command_NextToken(arg, num, sizeof(num));
    double amount = 0;
    if (!VTL_vkbot_command_ParseAmount(num, &amount)) {
        VTL_vkbot_command_Reply(token, peer, "Формат: /spend <сумма> <описание>");
        return;
    }
    long id = VTL_vkbot_store_AddOp(store, peer, amount, 0, desc);
    if (id > 0) snprintf(reply, sizeof(reply), "Записал расход #%ld: %.2f", id, amount);
    else        snprintf(reply, sizeof(reply), "Не удалось записать расход.");
    VTL_vkbot_command_Reply(token, peer, reply);
}

/* [2] /income <сумма> <описание> — записать доход. */
static void VTL_vkbot_command_HandleIncome(VTL_vkbot_Store* store, const char* token,
                                           const char* peer, const char* arg)
{
    char reply[VTL_VKBOT_REPLY_MAX];
    char num[32];
    const char* desc = VTL_vkbot_command_NextToken(arg, num, sizeof(num));
    double amount = 0;
    if (!VTL_vkbot_command_ParseAmount(num, &amount)) {
        VTL_vkbot_command_Reply(token, peer, "Формат: /income <сумма> <описание>");
        return;
    }
    long id = VTL_vkbot_store_AddOp(store, peer, amount, 1, desc);
    if (id > 0) snprintf(reply, sizeof(reply), "Записал доход #%ld: %.2f", id, amount);
    else        snprintf(reply, sizeof(reply), "Не удалось записать доход.");
    VTL_vkbot_command_Reply(token, peer, reply);
}

/* [3] /balance — текущий баланс. */
static void VTL_vkbot_command_HandleBalance(VTL_vkbot_Store* store, const char* token,
                                            const char* peer, const char* arg)
{
    (void)arg;
    char reply[VTL_VKBOT_REPLY_MAX];
    VTL_vkbot_store_FormatBalance(store, peer, reply, sizeof(reply));
    VTL_vkbot_command_Reply(token, peer, reply);
}

/* [4] /list — все операции. */
static void VTL_vkbot_command_HandleList(VTL_vkbot_Store* store, const char* token,
                                         const char* peer, const char* arg)
{
    (void)arg;
    char reply[VTL_VKBOT_REPLY_MAX];
    VTL_vkbot_store_ListOps(store, peer, reply, sizeof(reply));
    VTL_vkbot_command_Reply(token, peer, reply);
}

/* [5] /expenses — только расходы. */
static void VTL_vkbot_command_HandleExpenses(VTL_vkbot_Store* store, const char* token,
                                             const char* peer, const char* arg)
{
    (void)arg;
    char reply[VTL_VKBOT_REPLY_MAX];
    VTL_vkbot_store_ListByKind(store, peer, 0, reply, sizeof(reply));
    VTL_vkbot_command_Reply(token, peer, reply);
}

/* [6] /incomes — только доходы. */
static void VTL_vkbot_command_HandleIncomes(VTL_vkbot_Store* store, const char* token,
                                            const char* peer, const char* arg)
{
    (void)arg;
    char reply[VTL_VKBOT_REPLY_MAX];
    VTL_vkbot_store_ListByKind(store, peer, 1, reply, sizeof(reply));
    VTL_vkbot_command_Reply(token, peer, reply);
}

/* [7] /edit <N> <сумма> — изменить сумму операции. */
static void VTL_vkbot_command_HandleEdit(VTL_vkbot_Store* store, const char* token,
                                         const char* peer, const char* arg)
{
    char reply[VTL_VKBOT_REPLY_MAX];
    char num[32], amt[32];
    const char* rest = VTL_vkbot_command_NextToken(arg, num, sizeof(num));
    VTL_vkbot_command_NextToken(rest, amt, sizeof(amt));
    long id = strtol(num, NULL, 10);
    double amount = 0;
    if (id <= 0 || !VTL_vkbot_command_ParseAmount(amt, &amount)) {
        VTL_vkbot_command_Reply(token, peer, "Формат: /edit <N> <сумма>");
        return;
    }
    int ok = VTL_vkbot_store_EditOpAmount(store, peer, id, amount);
    snprintf(reply, sizeof(reply), ok ? "Сумма операции #%ld: %.2f"
                                      : "Операция #%ld не найдена.", id, amount);
    VTL_vkbot_command_Reply(token, peer, reply);
}

/* [8] /desc <N> <текст> — изменить описание операции. */
static void VTL_vkbot_command_HandleDesc(VTL_vkbot_Store* store, const char* token,
                                         const char* peer, const char* arg)
{
    char reply[VTL_VKBOT_REPLY_MAX];
    char num[32];
    const char* desc = VTL_vkbot_command_NextToken(arg, num, sizeof(num));
    long id = strtol(num, NULL, 10);
    if (id <= 0 || !*desc) {
        VTL_vkbot_command_Reply(token, peer, "Формат: /desc <N> <текст>");
        return;
    }
    int ok = VTL_vkbot_store_EditOpDesc(store, peer, id, desc);
    snprintf(reply, sizeof(reply), ok ? "Описание операции #%ld изменено."
                                      : "Операция #%ld не найдена.", id);
    VTL_vkbot_command_Reply(token, peer, reply);
}

/* [9] /del <N> — удалить операцию. */
static void VTL_vkbot_command_HandleDel(VTL_vkbot_Store* store, const char* token,
                                        const char* peer, const char* arg)
{
    char reply[VTL_VKBOT_REPLY_MAX];
    long id = strtol(arg, NULL, 10);
    if (id <= 0) {
        VTL_vkbot_command_Reply(token, peer, "Укажи номер: /del <N>");
        return;
    }
    int ok = VTL_vkbot_store_DelOp(store, peer, id);
    snprintf(reply, sizeof(reply), ok ? "Операция #%ld удалена."
                                      : "Операция #%ld не найдена.", id);
    VTL_vkbot_command_Reply(token, peer, reply);
}

/* [10] /clear — удалить все операции. */
static void VTL_vkbot_command_HandleClear(VTL_vkbot_Store* store, const char* token,
                                          const char* peer, const char* arg)
{
    (void)arg;
    char reply[VTL_VKBOT_REPLY_MAX];
    int removed = VTL_vkbot_store_ClearOps(store, peer);
    snprintf(reply, sizeof(reply), "Удалил операций: %d.", removed);
    VTL_vkbot_command_Reply(token, peer, reply);
}

/* [11] /find <текст> — поиск по описанию. */
static void VTL_vkbot_command_HandleFind(VTL_vkbot_Store* store, const char* token,
                                         const char* peer, const char* arg)
{
    char reply[VTL_VKBOT_REPLY_MAX];
    if (!*arg) {
        VTL_vkbot_command_Reply(token, peer, "Что искать? /find <текст>");
        return;
    }
    VTL_vkbot_store_SearchOps(store, peer, arg, reply, sizeof(reply));
    VTL_vkbot_command_Reply(token, peer, reply);
}

/* [12] /first — первая операция. */
static void VTL_vkbot_command_HandleFirst(VTL_vkbot_Store* store, const char* token,
                                          const char* peer, const char* arg)
{
    (void)arg;
    char reply[VTL_VKBOT_REPLY_MAX];
    VTL_vkbot_store_FormatEdgeOp(store, peer, 0, reply, sizeof(reply));
    VTL_vkbot_command_Reply(token, peer, reply);
}

/* [13] /last — последняя операция. */
static void VTL_vkbot_command_HandleLast(VTL_vkbot_Store* store, const char* token,
                                         const char* peer, const char* arg)
{
    (void)arg;
    char reply[VTL_VKBOT_REPLY_MAX];
    VTL_vkbot_store_FormatEdgeOp(store, peer, 1, reply, sizeof(reply));
    VTL_vkbot_command_Reply(token, peer, reply);
}

/* [14] /count — число операций. */
static void VTL_vkbot_command_HandleCount(VTL_vkbot_Store* store, const char* token,
                                          const char* peer, const char* arg)
{
    (void)arg;
    char reply[VTL_VKBOT_REPLY_MAX];
    int total = VTL_vkbot_store_CountOps(store, peer);
    snprintf(reply, sizeof(reply), "Операций: %d.", total);
    VTL_vkbot_command_Reply(token, peer, reply);
}

/* [15] /stats — сводка. */
static void VTL_vkbot_command_HandleStats(VTL_vkbot_Store* store, const char* token,
                                          const char* peer, const char* arg)
{
    (void)arg;
    char reply[VTL_VKBOT_REPLY_MAX];
    VTL_vkbot_store_Stats(store, peer, reply, sizeof(reply));
    VTL_vkbot_command_Reply(token, peer, reply);
}

/* [16] /export — выгрузить операции списком. */
static void VTL_vkbot_command_HandleExport(VTL_vkbot_Store* store, const char* token,
                                           const char* peer, const char* arg)
{
    (void)arg;
    char reply[VTL_VKBOT_REPLY_MAX];
    VTL_vkbot_store_ExportOps(store, peer, reply, sizeof(reply));
    VTL_vkbot_command_Reply(token, peer, reply);
}

/* [17] /today — операции за сегодня. */
static void VTL_vkbot_command_HandleToday(VTL_vkbot_Store* store, const char* token,
                                          const char* peer, const char* arg)
{
    (void)arg;
    char reply[VTL_VKBOT_REPLY_MAX];
    VTL_vkbot_store_ListToday(store, peer, time(NULL), reply, sizeof(reply));
    VTL_vkbot_command_Reply(token, peer, reply);
}

/* [18] /max — крупнейший расход. */
static void VTL_vkbot_command_HandleMax(VTL_vkbot_Store* store, const char* token,
                                        const char* peer, const char* arg)
{
    (void)arg;
    char reply[VTL_VKBOT_REPLY_MAX];
    VTL_vkbot_store_FormatMaxExpense(store, peer, reply, sizeof(reply));
    VTL_vkbot_command_Reply(token, peer, reply);
}

/* [19] /avg — средний расход. */
static void VTL_vkbot_command_HandleAvg(VTL_vkbot_Store* store, const char* token,
                                        const char* peer, const char* arg)
{
    (void)arg;
    char reply[VTL_VKBOT_REPLY_MAX];
    VTL_vkbot_store_FormatAvgExpense(store, peer, reply, sizeof(reply));
    VTL_vkbot_command_Reply(token, peer, reply);
}

/* [20] /setcat <N> <категория> — присвоить категорию операции. */
static void VTL_vkbot_command_HandleSetCat(VTL_vkbot_Store* store, const char* token,
                                           const char* peer, const char* arg)
{
    char reply[VTL_VKBOT_REPLY_MAX];
    char num[32];
    const char* cat = VTL_vkbot_command_NextToken(arg, num, sizeof(num));
    long id = strtol(num, NULL, 10);
    if (id <= 0 || !*cat) {
        VTL_vkbot_command_Reply(token, peer, "Формат: /setcat <N> <категория>");
        return;
    }
    int ok = VTL_vkbot_store_SetOpCat(store, peer, id, cat);
    if (ok) snprintf(reply, sizeof(reply), "Категория операции #%ld: %s", id, cat);
    else    snprintf(reply, sizeof(reply), "Операция #%ld не найдена.", id);
    VTL_vkbot_command_Reply(token, peer, reply);
}

/* [21] /bycat <категория> — операции по категории. */
static void VTL_vkbot_command_HandleByCat(VTL_vkbot_Store* store, const char* token,
                                          const char* peer, const char* arg)
{
    char reply[VTL_VKBOT_REPLY_MAX];
    if (!*arg) {
        VTL_vkbot_command_Reply(token, peer, "Формат: /bycat <категория>");
        return;
    }
    VTL_vkbot_store_ListByCat(store, peer, arg, reply, sizeof(reply));
    VTL_vkbot_command_Reply(token, peer, reply);
}

/* [22] /addcat <название> — добавить категорию. */
static void VTL_vkbot_command_HandleAddCat(VTL_vkbot_Store* store, const char* token,
                                           const char* peer, const char* arg)
{
    char reply[VTL_VKBOT_REPLY_MAX];
    if (!*arg) {
        VTL_vkbot_command_Reply(token, peer, "Как назвать? /addcat <название>");
        return;
    }
    long id = VTL_vkbot_store_AddCat(store, peer, arg);
    if (id > 0) snprintf(reply, sizeof(reply), "Добавил категорию #%ld: %s", id, arg);
    else        snprintf(reply, sizeof(reply), "Не удалось добавить категорию.");
    VTL_vkbot_command_Reply(token, peer, reply);
}

/* [23] /cats — список категорий. */
static void VTL_vkbot_command_HandleCats(VTL_vkbot_Store* store, const char* token,
                                         const char* peer, const char* arg)
{
    (void)arg;
    char reply[VTL_VKBOT_REPLY_MAX];
    VTL_vkbot_store_ListCats(store, peer, reply, sizeof(reply));
    VTL_vkbot_command_Reply(token, peer, reply);
}

/* [24] /delcat <N> — удалить категорию. */
static void VTL_vkbot_command_HandleDelCat(VTL_vkbot_Store* store, const char* token,
                                           const char* peer, const char* arg)
{
    char reply[VTL_VKBOT_REPLY_MAX];
    long id = strtol(arg, NULL, 10);
    if (id <= 0) {
        VTL_vkbot_command_Reply(token, peer, "Укажи номер: /delcat <N>");
        return;
    }
    int ok = VTL_vkbot_store_DelCat(store, peer, id);
    snprintf(reply, sizeof(reply), ok ? "Категория #%ld удалена."
                                      : "Категория #%ld не найдена.", id);
    VTL_vkbot_command_Reply(token, peer, reply);
}

/* [25] /budget <сумма> — задать лимит расходов. */
static void VTL_vkbot_command_HandleBudget(VTL_vkbot_Store* store, const char* token,
                                           const char* peer, const char* arg)
{
    char reply[VTL_VKBOT_REPLY_MAX];
    double amount = 0;
    if (!VTL_vkbot_command_ParseAmount(arg, &amount)) {
        VTL_vkbot_command_Reply(token, peer, "Формат: /budget <сумма>");
        return;
    }
    VTL_vkbot_store_SetBudget(store, peer, amount);
    snprintf(reply, sizeof(reply), "Лимит расходов: %.2f", amount);
    VTL_vkbot_command_Reply(token, peer, reply);
}

/* [26] /left — остаток лимита. */
static void VTL_vkbot_command_HandleLeft(VTL_vkbot_Store* store, const char* token,
                                         const char* peer, const char* arg)
{
    (void)arg;
    char reply[VTL_VKBOT_REPLY_MAX];
    VTL_vkbot_store_FormatBudgetLeft(store, peer, reply, sizeof(reply));
    VTL_vkbot_command_Reply(token, peer, reply);
}

/* [27] /id — показать peer_id. */
static void VTL_vkbot_command_HandleId(VTL_vkbot_Store* store, const char* token,
                                       const char* peer, const char* arg)
{
    (void)store; (void)arg;
    char reply[VTL_VKBOT_REPLY_MAX];
    snprintf(reply, sizeof(reply), "Твой peer_id: %s", peer);
    VTL_vkbot_command_Reply(token, peer, reply);
}

/* [28] /time — текущее время сервера. */
static void VTL_vkbot_command_HandleTime(VTL_vkbot_Store* store, const char* token,
                                         const char* peer, const char* arg)
{
    (void)store; (void)arg;
    char reply[VTL_VKBOT_REPLY_MAX];
    char ts[32];
    VTL_vkbot_command_FormatTime(time(NULL), ts, sizeof(ts));
    snprintf(reply, sizeof(reply), "Сейчас: %s", ts);
    VTL_vkbot_command_Reply(token, peer, reply);
}

/* [29] /ping — проверка связи. */
static void VTL_vkbot_command_HandlePing(VTL_vkbot_Store* store, const char* token,
                                         const char* peer, const char* arg)
{
    (void)store; (void)arg;
    VTL_vkbot_command_Reply(token, peer, "🏓 pong");
}

/* [30] /start, /help — справка по командам. */
static void VTL_vkbot_command_HandleHelp(VTL_vkbot_Store* store, const char* token,
                                         const char* peer, const char* arg)
{
    (void)store; (void)arg;
    VTL_vkbot_command_Reply(token, peer, VTL_vkbot_command_kHelp);
}

/* ===================== диспетчер команд ============================ */

typedef void (*VTL_vkbot_command_Fn)(VTL_vkbot_Store*, const char*,
                                     const char*, const char*);

typedef struct {
    const char*          name;
    VTL_vkbot_command_Fn fn;
} VTL_vkbot_command_Entry;

static const VTL_vkbot_command_Entry VTL_vkbot_command_kTable[] = {
    { "spend",    VTL_vkbot_command_HandleSpend    },
    { "income",   VTL_vkbot_command_HandleIncome   },
    { "balance",  VTL_vkbot_command_HandleBalance  },
    { "list",     VTL_vkbot_command_HandleList     },
    { "expenses", VTL_vkbot_command_HandleExpenses },
    { "incomes",  VTL_vkbot_command_HandleIncomes  },
    { "edit",     VTL_vkbot_command_HandleEdit     },
    { "desc",     VTL_vkbot_command_HandleDesc     },
    { "del",      VTL_vkbot_command_HandleDel      },
    { "clear",    VTL_vkbot_command_HandleClear    },
    { "find",     VTL_vkbot_command_HandleFind     },
    { "first",    VTL_vkbot_command_HandleFirst    },
    { "last",     VTL_vkbot_command_HandleLast     },
    { "count",    VTL_vkbot_command_HandleCount    },
    { "stats",    VTL_vkbot_command_HandleStats    },
    { "export",   VTL_vkbot_command_HandleExport   },
    { "today",    VTL_vkbot_command_HandleToday    },
    { "max",      VTL_vkbot_command_HandleMax      },
    { "avg",      VTL_vkbot_command_HandleAvg      },
    { "setcat",   VTL_vkbot_command_HandleSetCat   },
    { "bycat",    VTL_vkbot_command_HandleByCat    },
    { "addcat",   VTL_vkbot_command_HandleAddCat   },
    { "cats",     VTL_vkbot_command_HandleCats     },
    { "delcat",   VTL_vkbot_command_HandleDelCat   },
    { "budget",   VTL_vkbot_command_HandleBudget   },
    { "left",     VTL_vkbot_command_HandleLeft     },
    { "id",       VTL_vkbot_command_HandleId       },
    { "time",     VTL_vkbot_command_HandleTime     },
    { "ping",     VTL_vkbot_command_HandlePing     },
    { "start",    VTL_vkbot_command_HandleHelp     },
    { "help",     VTL_vkbot_command_HandleHelp     },
};

/* Извлекает имя команды (в нижнем регистре) из text в cmd; возвращает
 * указатель на аргументы после имени. */
static const char* VTL_vkbot_command_ParseName(const char* text, char* cmd, size_t cap)
{
    size_t n = 0;
    while (text[n] && !isspace((unsigned char)text[n]) && n + 1 < cap) {
        cmd[n] = (char)tolower((unsigned char)text[n]);
        ++n;
    }
    cmd[n] = '\0';
    return VTL_vkbot_command_Skip(text + n);
}

void VTL_vkbot_command_Handle(VTL_vkbot_Store* store, const char* token,
                              const char* peer_id, const char* text)
{
    if (!store || !token || !peer_id) return;

    /* Команды принимаем как со слешем, так и без него (привычнее в VK). */
    text = text ? VTL_vkbot_command_Skip(text) : "";
    if (*text == '/') ++text;
    if (!*text) {
        VTL_vkbot_command_Reply(token, peer_id,
            "Не понял. Напиши help, чтобы увидеть команды.");
        return;
    }

    char cmd[32];
    const char* arg = VTL_vkbot_command_ParseName(text, cmd, sizeof(cmd));

    const size_t count = sizeof(VTL_vkbot_command_kTable) /
                         sizeof(VTL_vkbot_command_kTable[0]);
    for (size_t i = 0; i < count; ++i) {
        if (strcmp(cmd, VTL_vkbot_command_kTable[i].name) == 0) {
            VTL_vkbot_command_kTable[i].fn(store, token, peer_id, arg);
            return;
        }
    }

    VTL_vkbot_command_Reply(token, peer_id,
        "Неизвестная команда. help — список команд.");
}
