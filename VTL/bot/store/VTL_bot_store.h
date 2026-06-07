#ifndef _VTL_BOT_STORE_H
#define _VTL_BOT_STORE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/VTL_app_result.h>
#include <stddef.h>
#include <time.h>

/*
 * Хранилище задач и напоминаний в JSON-файле (parson).
 *
 * Модель файла:
 *   {
 *     "chats": {
 *       "<chat_id>": {
 *         "next_task_id":     <число>,
 *         "next_reminder_id": <число>,
 *         "tasks":     [ {"id","text","done","created"}, ... ],
 *         "reminders": [ {"id","text","due","fired"},     ... ]
 *       }
 *     }
 *   }
 *
 * Данные разделены по chat_id — у каждого пользователя свой список.
 */

typedef struct VTL_bot_Store VTL_bot_Store;

/* Открывает (или создаёт пустое) хранилище из файла path. NULL — ошибка. */
VTL_bot_Store* VTL_bot_store_Open(const char* path);

/* Сохраняет (если были изменения) и освобождает хранилище. */
void VTL_bot_store_Close(VTL_bot_Store* store);

/* Принудительно сериализует хранилище в файл. */
VTL_AppResult VTL_bot_store_Save(VTL_bot_Store* store);

/* --- Задачи --- */

/* Добавляет задачу. Возвращает присвоенный id (>0) либо -1 при ошибке. */
long VTL_bot_store_AddTask(VTL_bot_Store* store, const char* chat_id,
                           const char* text);

/* Печатает список задач чата в out (cap байт), готовый к отправке. */
void VTL_bot_store_ListTasks(VTL_bot_Store* store, const char* chat_id,
                             char* out, size_t cap);

/* Отмечает задачу с данным id выполненной. 1 — найдена, 0 — нет. */
int VTL_bot_store_MarkDone(VTL_bot_Store* store, const char* chat_id, long id);

/* Удаляет задачу с данным id. 1 — удалена, 0 — не найдена. */
int VTL_bot_store_DelTask(VTL_bot_Store* store, const char* chat_id, long id);

/* Удаляет все выполненные задачи чата. Возвращает число удалённых. */
int VTL_bot_store_ClearDone(VTL_bot_Store* store, const char* chat_id);

/* Меняет текст задачи id. 1 — найдена и изменена, 0 — нет. */
int VTL_bot_store_EditTask(VTL_bot_Store* store, const char* chat_id,
                           long id, const char* new_text);

/* Считает задачи чата: возвращает всего, *out_done — число выполненных. */
int VTL_bot_store_CountTasks(VTL_bot_Store* store, const char* chat_id,
                             int* out_done);

/* Печатает задачи чата, чей текст содержит query, в out (cap байт). */
void VTL_bot_store_SearchTasks(VTL_bot_Store* store, const char* chat_id,
                               const char* query, char* out, size_t cap);

/* Печатает задачи с заданным статусом (done_flag: 1 — выполненные, 0 — нет). */
void VTL_bot_store_ListByStatus(VTL_bot_Store* store, const char* chat_id,
                                int done_flag, char* out, size_t cap);

/* Снимает отметку выполнения с задачи id. 1 — найдена, 0 — нет. */
int VTL_bot_store_MarkUndone(VTL_bot_Store* store, const char* chat_id, long id);

/* Удаляет все задачи чата. Возвращает число удалённых. */
int VTL_bot_store_ClearAll(VTL_bot_Store* store, const char* chat_id);

/* Задаёт приоритет (0–3) задачи id. 1 — найдена, 0 — нет. */
int VTL_bot_store_SetPriority(VTL_bot_Store* store, const char* chat_id,
                              long id, int prio);

/* Задаёт тег задачи id. 1 — найдена, 0 — нет. */
int VTL_bot_store_SetTag(VTL_bot_Store* store, const char* chat_id,
                         long id, const char* tag);

/* Печатает первую (want_last=0) или последнюю (want_last=1) задачу. */
void VTL_bot_store_FormatEdgeTask(VTL_bot_Store* store, const char* chat_id,
                                  int want_last, char* out, size_t cap);

/* Выгружает все задачи простым нумерованным списком. */
void VTL_bot_store_ExportTasks(VTL_bot_Store* store, const char* chat_id,
                               char* out, size_t cap);

/* --- Напоминания --- */

/* Добавляет напоминание на момент due. Возвращает id (>0) либо -1. */
long VTL_bot_store_AddReminder(VTL_bot_Store* store, const char* chat_id,
                               time_t due, const char* text);

/* Печатает несработавшие напоминания чата в out (cap байт). */
void VTL_bot_store_ListReminders(VTL_bot_Store* store, const char* chat_id,
                                 char* out, size_t cap);

/* Отменяет (удаляет) напоминание id. 1 — найдено, 0 — нет. */
int VTL_bot_store_CancelReminder(VTL_bot_Store* store, const char* chat_id,
                                 long id);

/* Удаляет все напоминания чата. Возвращает число удалённых. */
int VTL_bot_store_ClearReminders(VTL_bot_Store* store, const char* chat_id);

/* Печатает ближайшее несработавшее напоминание. */
void VTL_bot_store_NextReminder(VTL_bot_Store* store, const char* chat_id,
                                char* out, size_t cap);

/* --- Заметки --- */

/* Добавляет заметку. Возвращает id (>0) либо -1. */
long VTL_bot_store_AddNote(VTL_bot_Store* store, const char* chat_id,
                           const char* text);

/* Печатает список заметок чата в out (cap байт). */
void VTL_bot_store_ListNotes(VTL_bot_Store* store, const char* chat_id,
                             char* out, size_t cap);

/* Удаляет заметку id. 1 — найдена, 0 — нет. */
int VTL_bot_store_DelNote(VTL_bot_Store* store, const char* chat_id, long id);

/* Колбэк на сработавшее напоминание. */
typedef void (*VTL_bot_ReminderFn)(const char* chat_id, const char* text,
                                   void* user_data);

/* Вызывает fn для каждого напоминания с due <= now и помечает его сработавшим.
 * Возвращает число сработавших. */
int VTL_bot_store_FireDueReminders(VTL_bot_Store* store, time_t now,
                                   VTL_bot_ReminderFn fn, void* user_data);

#ifdef __cplusplus
}
#endif

#endif /* _VTL_BOT_STORE_H */
