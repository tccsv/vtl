/* localtime_r — POSIX; при -std=c11 объявление скрыто, явно просим POSIX. */
#if !defined(_WIN32)
#  define _POSIX_C_SOURCE 200809L
#endif

#include <VTL/bot/command/VTL_bot_command.h>
#include <VTL/bot/api/VTL_bot_api.h>
#include <VTL/bot/VTL_bot_data.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char* VTL_bot_command_kHelp =
    "Я бот для задач, напоминаний и заметок.\n"
    "\n"
    "Задачи:\n"
    "/add <текст> — добавить задачу\n"
    "/list — все задачи\n"
    "/pending — невыполненные\n"
    "/completed — выполненные\n"
    "/edit <N> <текст> — изменить задачу\n"
    "/done <N> — отметить выполненной\n"
    "/undone <N> — снять отметку\n"
    "/del <N> — удалить задачу\n"
    "/clear — убрать выполненные\n"
    "/clearall — удалить все задачи\n"
    "/find <текст> — поиск\n"
    "/prio <N> <0-3> — приоритет\n"
    "/tag <N> <тег> — тег\n"
    "/first — самая старая\n"
    "/last — самая новая\n"
    "/count — число задач\n"
    "/stats — статистика\n"
    "/export — выгрузить списком\n"
    "\n"
    "Напоминания:\n"
    "/remind <время> <текст> — напомнить (30s, 15m, 2h или 18:00)\n"
    "/reminders — список\n"
    "/next — ближайшее\n"
    "/cancel <N> — отменить\n"
    "/clearrem — удалить все\n"
    "\n"
    "Заметки:\n"
    "/note <текст> — добавить заметку\n"
    "/notes — список заметок\n"
    "/delnote <N> — удалить заметку\n"
    "\n"
    "Сервис:\n"
    "/id — мой chat_id\n"
    "/time — время сервера\n"
    "/ping — проверка связи\n"
    "/help — эта справка";

/* ============================ помощники ============================ */

/* Пропускает ведущие пробелы. */
static const char* VTL_bot_command_Skip(const char* s)
{
    while (*s && isspace((unsigned char)*s)) ++s;
    return s;
}

/* Копирует первый токен (до пробела) из s в tok; возвращает остаток строки. */
static const char* VTL_bot_command_NextToken(const char* s, char* tok, size_t cap)
{
    s = VTL_bot_command_Skip(s);
    size_t n = 0;
    while (s[n] && !isspace((unsigned char)s[n]) && n + 1 < cap) {
        tok[n] = s[n];
        ++n;
    }
    tok[n] = '\0';
    return VTL_bot_command_Skip(s + n);
}

/* Форматирует момент времени как "дд.мм ЧЧ:ММ". */
static void VTL_bot_command_FormatTime(time_t t, char* out, size_t cap)
{
    struct tm tm_t;
#if defined(_WIN32)
    localtime_s(&tm_t, &t);
#else
    localtime_r(&t, &tm_t);
#endif
    strftime(out, cap, "%d.%m %H:%M", &tm_t);
}

/*
 * Разбирает строку времени в абсолютный момент относительно now:
 *   "<число>s|m|h" — смещение в секундах/минутах/часах;
 *   "<число>"      — то же в минутах;
 *   "HH:MM"        — ближайшее наступление этого времени (сегодня/завтра).
 * Возвращает 1 при успехе (через out), 0 при ошибке.
 */
static int VTL_bot_command_ParseWhen(const char* arg, time_t now, time_t* out)
{
    if (!arg || !*arg || !out) return 0;

    const char* colon = strchr(arg, ':');
    if (colon) {
        int hh = -1, mm = -1;
        if (sscanf(arg, "%d:%d", &hh, &mm) == 2 &&
            hh >= 0 && hh < 24 && mm >= 0 && mm < 60) {
            struct tm tm_now;
#if defined(_WIN32)
            localtime_s(&tm_now, &now);
#else
            localtime_r(&now, &tm_now);
#endif
            tm_now.tm_hour = hh;
            tm_now.tm_min  = mm;
            tm_now.tm_sec  = 0;
            time_t due = mktime(&tm_now);
            if (due <= now) due += 24 * 60 * 60; /* уже прошло — на завтра */
            *out = due;
            return 1;
        }
        return 0;
    }

    char* end = NULL;
    long value = strtol(arg, &end, 10);
    if (end == arg || value <= 0) return 0;

    long mult = 60; /* по умолчанию минуты */
    if (*end == 's' || *end == 'S') mult = 1;
    else if (*end == 'm' || *end == 'M') mult = 60;
    else if (*end == 'h' || *end == 'H') mult = 60 * 60;
    else if (*end != '\0') return 0;

    *out = now + (time_t)value * mult;
    return 1;
}

/* Отправляет text в чат; ошибки транспорта только логируются. */
static void VTL_bot_command_Reply(const char* token, const char* chat_id,
                                  const char* text)
{
    if (VTL_bot_api_SendMessage(token, chat_id, text) != VTL_res_kOk)
        fprintf(stderr, "[bot] sendMessage в чат %s не удался\n", chat_id);
}

/* ===================== обработчики команд ===========================
 * Каждый метод отвечает ровно за одну команду. Единая сигнатура —
 * чтобы разложить их в таблицу-диспетчер ниже. arg — аргументы после
 * имени команды (без ведущих пробелов). Нумерация [N] — функционал.
 * =================================================================== */

/* [1] /add <текст> — добавить новую задачу. */
static void VTL_bot_command_HandleAdd(VTL_bot_Store* store, const char* token,
                                      const char* chat_id, const char* arg)
{
    char reply[VTL_BOT_REPLY_MAX];
    if (!*arg) {
        VTL_bot_command_Reply(token, chat_id, "Что добавить? /add <текст>");
        return;
    }
    long id = VTL_bot_store_AddTask(store, chat_id, arg);
    if (id > 0) snprintf(reply, sizeof(reply), "Добавил задачу #%ld.", id);
    else        snprintf(reply, sizeof(reply), "Не удалось добавить задачу.");
    VTL_bot_command_Reply(token, chat_id, reply);
}

/* [2] /list — показать все задачи чата. */
static void VTL_bot_command_HandleList(VTL_bot_Store* store, const char* token,
                                       const char* chat_id, const char* arg)
{
    (void)arg;
    char reply[VTL_BOT_REPLY_MAX];
    VTL_bot_store_ListTasks(store, chat_id, reply, sizeof(reply));
    VTL_bot_command_Reply(token, chat_id, reply);
}

/* [3] /pending — показать невыполненные задачи. */
static void VTL_bot_command_HandlePending(VTL_bot_Store* store, const char* token,
                                          const char* chat_id, const char* arg)
{
    (void)arg;
    char reply[VTL_BOT_REPLY_MAX];
    VTL_bot_store_ListByStatus(store, chat_id, 0, reply, sizeof(reply));
    VTL_bot_command_Reply(token, chat_id, reply);
}

/* [4] /completed — показать выполненные задачи. */
static void VTL_bot_command_HandleCompleted(VTL_bot_Store* store, const char* token,
                                            const char* chat_id, const char* arg)
{
    (void)arg;
    char reply[VTL_BOT_REPLY_MAX];
    VTL_bot_store_ListByStatus(store, chat_id, 1, reply, sizeof(reply));
    VTL_bot_command_Reply(token, chat_id, reply);
}

/* [5] /edit <N> <текст> — изменить текст задачи N. */
static void VTL_bot_command_HandleEdit(VTL_bot_Store* store, const char* token,
                                       const char* chat_id, const char* arg)
{
    char reply[VTL_BOT_REPLY_MAX];
    char num[16];
    const char* text = VTL_bot_command_NextToken(arg, num, sizeof(num));
    long id = strtol(num, NULL, 10);
    if (id <= 0 || !*text) {
        VTL_bot_command_Reply(token, chat_id, "Формат: /edit <N> <новый текст>");
        return;
    }
    int ok = VTL_bot_store_EditTask(store, chat_id, id, text);
    snprintf(reply, sizeof(reply), ok ? "Задача #%ld изменена."
                                      : "Задача #%ld не найдена.", id);
    VTL_bot_command_Reply(token, chat_id, reply);
}

/* [6] /done <N> — отметить задачу N выполненной. */
static void VTL_bot_command_HandleDone(VTL_bot_Store* store, const char* token,
                                       const char* chat_id, const char* arg)
{
    char reply[VTL_BOT_REPLY_MAX];
    long id = strtol(arg, NULL, 10);
    if (id <= 0) {
        VTL_bot_command_Reply(token, chat_id, "Укажи номер: /done <N>");
        return;
    }
    int ok = VTL_bot_store_MarkDone(store, chat_id, id);
    snprintf(reply, sizeof(reply), ok ? "Задача #%ld выполнена."
                                      : "Задача #%ld не найдена.", id);
    VTL_bot_command_Reply(token, chat_id, reply);
}

/* [7] /undone <N> — снять отметку выполнения с задачи N. */
static void VTL_bot_command_HandleUndone(VTL_bot_Store* store, const char* token,
                                         const char* chat_id, const char* arg)
{
    char reply[VTL_BOT_REPLY_MAX];
    long id = strtol(arg, NULL, 10);
    if (id <= 0) {
        VTL_bot_command_Reply(token, chat_id, "Укажи номер: /undone <N>");
        return;
    }
    int ok = VTL_bot_store_MarkUndone(store, chat_id, id);
    snprintf(reply, sizeof(reply), ok ? "Задача #%ld снова активна."
                                      : "Задача #%ld не найдена.", id);
    VTL_bot_command_Reply(token, chat_id, reply);
}

/* [8] /del <N> — удалить задачу N. */
static void VTL_bot_command_HandleDel(VTL_bot_Store* store, const char* token,
                                      const char* chat_id, const char* arg)
{
    char reply[VTL_BOT_REPLY_MAX];
    long id = strtol(arg, NULL, 10);
    if (id <= 0) {
        VTL_bot_command_Reply(token, chat_id, "Укажи номер: /del <N>");
        return;
    }
    int ok = VTL_bot_store_DelTask(store, chat_id, id);
    snprintf(reply, sizeof(reply), ok ? "Задача #%ld удалена."
                                      : "Задача #%ld не найдена.", id);
    VTL_bot_command_Reply(token, chat_id, reply);
}

/* [9] /clear — удалить все выполненные задачи. */
static void VTL_bot_command_HandleClear(VTL_bot_Store* store, const char* token,
                                        const char* chat_id, const char* arg)
{
    (void)arg;
    char reply[VTL_BOT_REPLY_MAX];
    int removed = VTL_bot_store_ClearDone(store, chat_id);
    snprintf(reply, sizeof(reply), "Убрал выполненных: %d.", removed);
    VTL_bot_command_Reply(token, chat_id, reply);
}

/* [10] /clearall — удалить все задачи чата. */
static void VTL_bot_command_HandleClearAll(VTL_bot_Store* store, const char* token,
                                           const char* chat_id, const char* arg)
{
    (void)arg;
    char reply[VTL_BOT_REPLY_MAX];
    int removed = VTL_bot_store_ClearAll(store, chat_id);
    snprintf(reply, sizeof(reply), "Удалил задач: %d.", removed);
    VTL_bot_command_Reply(token, chat_id, reply);
}

/* [11] /find <текст> — найти задачи по подстроке. */
static void VTL_bot_command_HandleFind(VTL_bot_Store* store, const char* token,
                                       const char* chat_id, const char* arg)
{
    char reply[VTL_BOT_REPLY_MAX];
    if (!*arg) {
        VTL_bot_command_Reply(token, chat_id, "Что искать? /find <текст>");
        return;
    }
    VTL_bot_store_SearchTasks(store, chat_id, arg, reply, sizeof(reply));
    VTL_bot_command_Reply(token, chat_id, reply);
}

/* [12] /prio <N> <0-3> — задать приоритет задачи N. */
static void VTL_bot_command_HandlePrio(VTL_bot_Store* store, const char* token,
                                       const char* chat_id, const char* arg)
{
    char reply[VTL_BOT_REPLY_MAX];
    char num[16], pr[16];
    const char* rest = VTL_bot_command_NextToken(arg, num, sizeof(num));
    VTL_bot_command_NextToken(rest, pr, sizeof(pr));
    long id = strtol(num, NULL, 10);
    int  prio = (int)strtol(pr, NULL, 10);
    if (id <= 0 || !*pr || prio < 0 || prio > 3) {
        VTL_bot_command_Reply(token, chat_id, "Формат: /prio <N> <0-3>");
        return;
    }
    int ok = VTL_bot_store_SetPriority(store, chat_id, id, prio);
    if (ok) snprintf(reply, sizeof(reply), "Приоритет задачи #%ld: %d.", id, prio);
    else    snprintf(reply, sizeof(reply), "Задача #%ld не найдена.", id);
    VTL_bot_command_Reply(token, chat_id, reply);
}

/* [13] /tag <N> <тег> — задать тег задачи N. */
static void VTL_bot_command_HandleTag(VTL_bot_Store* store, const char* token,
                                      const char* chat_id, const char* arg)
{
    char reply[VTL_BOT_REPLY_MAX];
    char num[16];
    const char* tag = VTL_bot_command_NextToken(arg, num, sizeof(num));
    long id = strtol(num, NULL, 10);
    if (id <= 0 || !*tag) {
        VTL_bot_command_Reply(token, chat_id, "Формат: /tag <N> <тег>");
        return;
    }
    int ok = VTL_bot_store_SetTag(store, chat_id, id, tag);
    if (ok) snprintf(reply, sizeof(reply), "Тег задачи #%ld: %s", id, tag);
    else    snprintf(reply, sizeof(reply), "Задача #%ld не найдена.", id);
    VTL_bot_command_Reply(token, chat_id, reply);
}

/* [14] /first — показать самую старую задачу. */
static void VTL_bot_command_HandleFirst(VTL_bot_Store* store, const char* token,
                                        const char* chat_id, const char* arg)
{
    (void)arg;
    char reply[VTL_BOT_REPLY_MAX];
    VTL_bot_store_FormatEdgeTask(store, chat_id, 0, reply, sizeof(reply));
    VTL_bot_command_Reply(token, chat_id, reply);
}

/* [15] /last — показать самую новую задачу. */
static void VTL_bot_command_HandleLast(VTL_bot_Store* store, const char* token,
                                       const char* chat_id, const char* arg)
{
    (void)arg;
    char reply[VTL_BOT_REPLY_MAX];
    VTL_bot_store_FormatEdgeTask(store, chat_id, 1, reply, sizeof(reply));
    VTL_bot_command_Reply(token, chat_id, reply);
}

/* [16] /count — короткий счётчик задач. */
static void VTL_bot_command_HandleCount(VTL_bot_Store* store, const char* token,
                                        const char* chat_id, const char* arg)
{
    (void)arg;
    char reply[VTL_BOT_REPLY_MAX];
    int done = 0;
    int total = VTL_bot_store_CountTasks(store, chat_id, &done);
    snprintf(reply, sizeof(reply), "Задач: %d (активных %d).", total, total - done);
    VTL_bot_command_Reply(token, chat_id, reply);
}

/* [17] /stats — статистика: всего / выполнено / осталось. */
static void VTL_bot_command_HandleStats(VTL_bot_Store* store, const char* token,
                                        const char* chat_id, const char* arg)
{
    (void)arg;
    char reply[VTL_BOT_REPLY_MAX];
    int done = 0;
    int total = VTL_bot_store_CountTasks(store, chat_id, &done);
    snprintf(reply, sizeof(reply),
             "Всего задач: %d\nВыполнено: %d\nОсталось: %d",
             total, done, total - done);
    VTL_bot_command_Reply(token, chat_id, reply);
}

/* [18] /export — выгрузить все задачи простым списком. */
static void VTL_bot_command_HandleExport(VTL_bot_Store* store, const char* token,
                                         const char* chat_id, const char* arg)
{
    (void)arg;
    char reply[VTL_BOT_REPLY_MAX];
    VTL_bot_store_ExportTasks(store, chat_id, reply, sizeof(reply));
    VTL_bot_command_Reply(token, chat_id, reply);
}

/* [19] /remind <время> <текст> — создать напоминание. */
static void VTL_bot_command_HandleRemind(VTL_bot_Store* store, const char* token,
                                         const char* chat_id, const char* arg)
{
    char reply[VTL_BOT_REPLY_MAX];
    char when[64];
    const char* note = VTL_bot_command_NextToken(arg, when, sizeof(when));

    time_t due = 0;
    if (!*when || !VTL_bot_command_ParseWhen(when, time(NULL), &due)) {
        VTL_bot_command_Reply(token, chat_id,
            "Формат: /remind <время> <текст>. Время: 30s, 15m, 2h или 18:00.");
        return;
    }
    if (!*note) {
        VTL_bot_command_Reply(token, chat_id, "О чём напомнить? Добавь текст.");
        return;
    }
    long id = VTL_bot_store_AddReminder(store, chat_id, due, note);
    if (id > 0) {
        char ts[32];
        VTL_bot_command_FormatTime(due, ts, sizeof(ts));
        snprintf(reply, sizeof(reply), "Напомню %s: %s", ts, note);
    } else {
        snprintf(reply, sizeof(reply), "Не удалось создать напоминание.");
    }
    VTL_bot_command_Reply(token, chat_id, reply);
}

/* [20] /reminders — показать активные напоминания. */
static void VTL_bot_command_HandleReminders(VTL_bot_Store* store, const char* token,
                                            const char* chat_id, const char* arg)
{
    (void)arg;
    char reply[VTL_BOT_REPLY_MAX];
    VTL_bot_store_ListReminders(store, chat_id, reply, sizeof(reply));
    VTL_bot_command_Reply(token, chat_id, reply);
}

/* [21] /next — показать ближайшее напоминание. */
static void VTL_bot_command_HandleNext(VTL_bot_Store* store, const char* token,
                                       const char* chat_id, const char* arg)
{
    (void)arg;
    char reply[VTL_BOT_REPLY_MAX];
    VTL_bot_store_NextReminder(store, chat_id, reply, sizeof(reply));
    VTL_bot_command_Reply(token, chat_id, reply);
}

/* [22] /cancel <N> — отменить напоминание N. */
static void VTL_bot_command_HandleCancel(VTL_bot_Store* store, const char* token,
                                         const char* chat_id, const char* arg)
{
    char reply[VTL_BOT_REPLY_MAX];
    long id = strtol(arg, NULL, 10);
    if (id <= 0) {
        VTL_bot_command_Reply(token, chat_id, "Укажи номер: /cancel <N>");
        return;
    }
    int ok = VTL_bot_store_CancelReminder(store, chat_id, id);
    snprintf(reply, sizeof(reply), ok ? "Напоминание #%ld отменено."
                                      : "Напоминание #%ld не найдено.", id);
    VTL_bot_command_Reply(token, chat_id, reply);
}

/* [23] /clearrem — удалить все напоминания. */
static void VTL_bot_command_HandleClearRem(VTL_bot_Store* store, const char* token,
                                           const char* chat_id, const char* arg)
{
    (void)arg;
    char reply[VTL_BOT_REPLY_MAX];
    int removed = VTL_bot_store_ClearReminders(store, chat_id);
    snprintf(reply, sizeof(reply), "Удалил напоминаний: %d.", removed);
    VTL_bot_command_Reply(token, chat_id, reply);
}

/* [24] /note <текст> — добавить заметку. */
static void VTL_bot_command_HandleNote(VTL_bot_Store* store, const char* token,
                                       const char* chat_id, const char* arg)
{
    char reply[VTL_BOT_REPLY_MAX];
    if (!*arg) {
        VTL_bot_command_Reply(token, chat_id, "Что записать? /note <текст>");
        return;
    }
    long id = VTL_bot_store_AddNote(store, chat_id, arg);
    if (id > 0) snprintf(reply, sizeof(reply), "Записал заметку #%ld.", id);
    else        snprintf(reply, sizeof(reply), "Не удалось добавить заметку.");
    VTL_bot_command_Reply(token, chat_id, reply);
}

/* [25] /notes — показать заметки. */
static void VTL_bot_command_HandleNotes(VTL_bot_Store* store, const char* token,
                                        const char* chat_id, const char* arg)
{
    (void)arg;
    char reply[VTL_BOT_REPLY_MAX];
    VTL_bot_store_ListNotes(store, chat_id, reply, sizeof(reply));
    VTL_bot_command_Reply(token, chat_id, reply);
}

/* [26] /delnote <N> — удалить заметку N. */
static void VTL_bot_command_HandleDelNote(VTL_bot_Store* store, const char* token,
                                          const char* chat_id, const char* arg)
{
    char reply[VTL_BOT_REPLY_MAX];
    long id = strtol(arg, NULL, 10);
    if (id <= 0) {
        VTL_bot_command_Reply(token, chat_id, "Укажи номер: /delnote <N>");
        return;
    }
    int ok = VTL_bot_store_DelNote(store, chat_id, id);
    snprintf(reply, sizeof(reply), ok ? "Заметка #%ld удалена."
                                      : "Заметка #%ld не найдена.", id);
    VTL_bot_command_Reply(token, chat_id, reply);
}

/* [27] /id — показать chat_id пользователя. */
static void VTL_bot_command_HandleId(VTL_bot_Store* store, const char* token,
                                     const char* chat_id, const char* arg)
{
    (void)store; (void)arg;
    char reply[VTL_BOT_REPLY_MAX];
    snprintf(reply, sizeof(reply), "Твой chat_id: %s", chat_id);
    VTL_bot_command_Reply(token, chat_id, reply);
}

/* [28] /time — показать текущее время сервера. */
static void VTL_bot_command_HandleTime(VTL_bot_Store* store, const char* token,
                                       const char* chat_id, const char* arg)
{
    (void)store; (void)arg;
    char reply[VTL_BOT_REPLY_MAX];
    char ts[32];
    VTL_bot_command_FormatTime(time(NULL), ts, sizeof(ts));
    snprintf(reply, sizeof(reply), "Сейчас: %s", ts);
    VTL_bot_command_Reply(token, chat_id, reply);
}

/* [29] /ping — проверка связи (отвечает pong). */
static void VTL_bot_command_HandlePing(VTL_bot_Store* store, const char* token,
                                       const char* chat_id, const char* arg)
{
    (void)store; (void)arg;
    VTL_bot_command_Reply(token, chat_id, "🏓 pong");
}

/* [30] /start, /help — показать справку по командам. */
static void VTL_bot_command_HandleHelp(VTL_bot_Store* store, const char* token,
                                       const char* chat_id, const char* arg)
{
    (void)store; (void)arg;
    VTL_bot_command_Reply(token, chat_id, VTL_bot_command_kHelp);
}

/* ===================== диспетчер команд ============================ */

typedef void (*VTL_bot_command_Fn)(VTL_bot_Store*, const char*,
                                   const char*, const char*);

typedef struct {
    const char*        name;
    VTL_bot_command_Fn fn;
} VTL_bot_command_Entry;

static const VTL_bot_command_Entry VTL_bot_command_kTable[] = {
    { "add",       VTL_bot_command_HandleAdd       },
    { "list",      VTL_bot_command_HandleList      },
    { "pending",   VTL_bot_command_HandlePending   },
    { "completed", VTL_bot_command_HandleCompleted },
    { "edit",      VTL_bot_command_HandleEdit      },
    { "done",      VTL_bot_command_HandleDone      },
    { "undone",    VTL_bot_command_HandleUndone    },
    { "del",       VTL_bot_command_HandleDel       },
    { "clear",     VTL_bot_command_HandleClear     },
    { "clearall",  VTL_bot_command_HandleClearAll  },
    { "find",      VTL_bot_command_HandleFind      },
    { "prio",      VTL_bot_command_HandlePrio      },
    { "tag",       VTL_bot_command_HandleTag       },
    { "first",     VTL_bot_command_HandleFirst     },
    { "last",      VTL_bot_command_HandleLast      },
    { "count",     VTL_bot_command_HandleCount     },
    { "stats",     VTL_bot_command_HandleStats     },
    { "export",    VTL_bot_command_HandleExport    },
    { "remind",    VTL_bot_command_HandleRemind    },
    { "reminders", VTL_bot_command_HandleReminders },
    { "next",      VTL_bot_command_HandleNext      },
    { "cancel",    VTL_bot_command_HandleCancel    },
    { "clearrem",  VTL_bot_command_HandleClearRem  },
    { "note",      VTL_bot_command_HandleNote      },
    { "notes",     VTL_bot_command_HandleNotes     },
    { "delnote",   VTL_bot_command_HandleDelNote   },
    { "id",        VTL_bot_command_HandleId        },
    { "time",      VTL_bot_command_HandleTime      },
    { "ping",      VTL_bot_command_HandlePing      },
    { "start",     VTL_bot_command_HandleHelp      },
    { "help",      VTL_bot_command_HandleHelp      },
};

/* Извлекает имя команды (в нижнем регистре) из text в cmd; возвращает
 * указатель на аргументы после имени (и возможного @botname). */
static const char* VTL_bot_command_ParseName(const char* text, char* cmd, size_t cap)
{
    size_t n = 0;
    while (text[n] && !isspace((unsigned char)text[n]) && text[n] != '@'
           && n + 1 < cap) {
        cmd[n] = (char)tolower((unsigned char)text[n]);
        ++n;
    }
    cmd[n] = '\0';

    const char* rest = text + n;
    while (*rest && *rest != ' ') ++rest;        /* доедаем @botname */
    return VTL_bot_command_Skip(rest);
}

void VTL_bot_command_Handle(VTL_bot_Store* store, const char* token,
                            const char* chat_id, const char* text)
{
    if (!store || !token || !chat_id) return;

    text = text ? VTL_bot_command_Skip(text) : "";
    if (*text != '/') {
        VTL_bot_command_Reply(token, chat_id,
            "Не понял. Напиши /help, чтобы увидеть команды.");
        return;
    }

    char cmd[32];
    const char* arg = VTL_bot_command_ParseName(text + 1, cmd, sizeof(cmd));

    const size_t count = sizeof(VTL_bot_command_kTable) /
                         sizeof(VTL_bot_command_kTable[0]);
    for (size_t i = 0; i < count; ++i) {
        if (strcmp(cmd, VTL_bot_command_kTable[i].name) == 0) {
            VTL_bot_command_kTable[i].fn(store, token, chat_id, arg);
            return;
        }
    }

    VTL_bot_command_Reply(token, chat_id,
        "Неизвестная команда. /help — список команд.");
}
