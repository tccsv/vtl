/* localtime_r — POSIX; при -std=c11 объявление скрыто, явно просим POSIX. */
#if !defined(_WIN32)
#  define _POSIX_C_SOURCE 200809L
#endif

#include <VTL/bot/store/VTL_bot_store.h>
#include <VTL/bot/VTL_bot_data.h>

#include <parson/parson.h>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct VTL_bot_Store {
    JSON_Value* root;   /* корневой объект всего файла */
    char*       path;   /* путь к файлу хранилища */
    int         dirty;  /* есть несохранённые изменения */
};

/* Возвращает объект "chats", создавая его при отсутствии. */
static JSON_Object* VTL_bot_store_Chats(VTL_bot_Store* store)
{
    JSON_Object* root = json_value_get_object(store->root);
    JSON_Object* chats = json_object_get_object(root, "chats");
    if (!chats) {
        json_object_set_value(root, "chats", json_value_init_object());
        chats = json_object_get_object(root, "chats");
    }
    return chats;
}

/* Возвращает объект конкретного чата, создавая его и поля при отсутствии. */
static JSON_Object* VTL_bot_store_Chat(VTL_bot_Store* store, const char* chat_id)
{
    JSON_Object* chats = VTL_bot_store_Chats(store);
    if (!chats || !chat_id) return NULL;

    JSON_Object* chat = json_object_get_object(chats, chat_id);
    if (!chat) {
        json_object_set_value(chats, chat_id, json_value_init_object());
        chat = json_object_get_object(chats, chat_id);
        if (!chat) return NULL;
        json_object_set_number(chat, "next_task_id", 1);
        json_object_set_number(chat, "next_reminder_id", 1);
        json_object_set_value(chat, "tasks", json_value_init_array());
        json_object_set_value(chat, "reminders", json_value_init_array());
    }
    return chat;
}

VTL_bot_Store* VTL_bot_store_Open(const char* path)
{
    if (!path) return NULL;

    VTL_bot_Store* store = (VTL_bot_Store*)calloc(1, sizeof(*store));
    if (!store) return NULL;

    size_t path_len = strlen(path) + 1;
    store->path = (char*)malloc(path_len);
    if (!store->path) { free(store); return NULL; }
    memcpy(store->path, path, path_len);

    /* Пытаемся прочитать существующий файл; иначе — пустой объект. */
    store->root = json_parse_file(path);
    if (!store->root || json_value_get_type(store->root) != JSONObject) {
        if (store->root) json_value_free(store->root);
        store->root = json_value_init_object();
    }
    if (!store->root) { free(store->path); free(store); return NULL; }

    store->dirty = 0;
    return store;
}

VTL_AppResult VTL_bot_store_Save(VTL_bot_Store* store)
{
    if (!store) return VTL_res_kInvalidParamErr;
    JSON_Status st = json_serialize_to_file_pretty(store->root, store->path);
    if (st != JSONSuccess) return VTL_res_kFileWriteErr;
    store->dirty = 0;
    return VTL_res_kOk;
}

void VTL_bot_store_Close(VTL_bot_Store* store)
{
    if (!store) return;
    if (store->dirty) VTL_bot_store_Save(store);
    if (store->root) json_value_free(store->root);
    free(store->path);
    free(store);
}

long VTL_bot_store_AddTask(VTL_bot_Store* store, const char* chat_id,
                           const char* text)
{
    if (!store || !chat_id || !text) return -1;
    JSON_Object* chat = VTL_bot_store_Chat(store, chat_id);
    if (!chat) return -1;

    long id = (long)json_object_get_number(chat, "next_task_id");
    if (id <= 0) id = 1;

    JSON_Array* tasks = json_object_get_array(chat, "tasks");
    JSON_Value* tv = json_value_init_object();
    JSON_Object* to = json_value_get_object(tv);
    json_object_set_number(to, "id", (double)id);
    json_object_set_string(to, "text", text);
    json_object_set_boolean(to, "done", 0);
    json_object_set_number(to, "created", (double)time(NULL));
    json_array_append_value(tasks, tv);

    json_object_set_number(chat, "next_task_id", (double)(id + 1));
    store->dirty = 1;
    VTL_bot_store_Save(store);
    return id;
}

void VTL_bot_store_ListTasks(VTL_bot_Store* store, const char* chat_id,
                             char* out, size_t cap)
{
    if (!out || cap == 0) return;
    out[0] = '\0';
    if (!store || !chat_id) return;

    JSON_Object* chat = VTL_bot_store_Chat(store, chat_id);
    JSON_Array* tasks = chat ? json_object_get_array(chat, "tasks") : NULL;
    size_t count = tasks ? json_array_get_count(tasks) : 0;

    if (count == 0) {
        snprintf(out, cap, "Список задач пуст. Добавь: /add <текст>");
        return;
    }

    size_t off = 0;
    off += (size_t)snprintf(out + off, cap - off, "Твои задачи:\n");
    for (size_t i = 0; i < count && off < cap; ++i) {
        JSON_Object* t = json_array_get_object(tasks, i);
        long id = (long)json_object_get_number(t, "id");
        int done = json_object_get_boolean(t, "done") > 0;
        const char* text = json_object_get_string(t, "text");
        if (!text) text = "";
        off += (size_t)snprintf(out + off, cap - off, "%s #%ld %s\n",
                                done ? "\xe2\x98\x91" : "\xe2\x98\x90", /* ☑ / ☐ */
                                id, text);
    }
}

/* Ищет индекс задачи по id; -1 если нет. */
static int VTL_bot_store_FindTask(JSON_Array* tasks, long id)
{
    size_t count = tasks ? json_array_get_count(tasks) : 0;
    for (size_t i = 0; i < count; ++i) {
        JSON_Object* t = json_array_get_object(tasks, i);
        if ((long)json_object_get_number(t, "id") == id) return (int)i;
    }
    return -1;
}

int VTL_bot_store_MarkDone(VTL_bot_Store* store, const char* chat_id, long id)
{
    if (!store || !chat_id) return 0;
    JSON_Object* chat = VTL_bot_store_Chat(store, chat_id);
    JSON_Array* tasks = chat ? json_object_get_array(chat, "tasks") : NULL;
    int idx = VTL_bot_store_FindTask(tasks, id);
    if (idx < 0) return 0;

    json_object_set_boolean(json_array_get_object(tasks, (size_t)idx), "done", 1);
    store->dirty = 1;
    VTL_bot_store_Save(store);
    return 1;
}

int VTL_bot_store_DelTask(VTL_bot_Store* store, const char* chat_id, long id)
{
    if (!store || !chat_id) return 0;
    JSON_Object* chat = VTL_bot_store_Chat(store, chat_id);
    JSON_Array* tasks = chat ? json_object_get_array(chat, "tasks") : NULL;
    int idx = VTL_bot_store_FindTask(tasks, id);
    if (idx < 0) return 0;

    json_array_remove(tasks, (size_t)idx);
    store->dirty = 1;
    VTL_bot_store_Save(store);
    return 1;
}

int VTL_bot_store_ClearDone(VTL_bot_Store* store, const char* chat_id)
{
    if (!store || !chat_id) return 0;
    JSON_Object* chat = VTL_bot_store_Chat(store, chat_id);
    JSON_Array* tasks = chat ? json_object_get_array(chat, "tasks") : NULL;
    if (!tasks) return 0;

    int removed = 0;
    /* Идём с конца — удаление не сдвигает ещё не просмотренные элементы. */
    for (size_t i = json_array_get_count(tasks); i > 0; --i) {
        JSON_Object* t = json_array_get_object(tasks, i - 1);
        if (json_object_get_boolean(t, "done") > 0) {
            json_array_remove(tasks, i - 1);
            ++removed;
        }
    }
    if (removed) { store->dirty = 1; VTL_bot_store_Save(store); }
    return removed;
}

int VTL_bot_store_EditTask(VTL_bot_Store* store, const char* chat_id,
                           long id, const char* new_text)
{
    if (!store || !chat_id || !new_text) return 0;
    JSON_Object* chat = VTL_bot_store_Chat(store, chat_id);
    JSON_Array* tasks = chat ? json_object_get_array(chat, "tasks") : NULL;
    int idx = VTL_bot_store_FindTask(tasks, id);
    if (idx < 0) return 0;

    json_object_set_string(json_array_get_object(tasks, (size_t)idx),
                           "text", new_text);
    store->dirty = 1;
    VTL_bot_store_Save(store);
    return 1;
}

int VTL_bot_store_CountTasks(VTL_bot_Store* store, const char* chat_id,
                             int* out_done)
{
    if (out_done) *out_done = 0;
    if (!store || !chat_id) return 0;

    JSON_Object* chat = VTL_bot_store_Chat(store, chat_id);
    JSON_Array* tasks = chat ? json_object_get_array(chat, "tasks") : NULL;
    size_t count = tasks ? json_array_get_count(tasks) : 0;

    int done = 0;
    for (size_t i = 0; i < count; ++i) {
        if (json_object_get_boolean(json_array_get_object(tasks, i), "done") > 0)
            ++done;
    }
    if (out_done) *out_done = done;
    return (int)count;
}

void VTL_bot_store_SearchTasks(VTL_bot_Store* store, const char* chat_id,
                               const char* query, char* out, size_t cap)
{
    if (!out || cap == 0) return;
    out[0] = '\0';
    if (!store || !chat_id || !query || !*query) return;

    JSON_Object* chat = VTL_bot_store_Chat(store, chat_id);
    JSON_Array* tasks = chat ? json_object_get_array(chat, "tasks") : NULL;
    size_t count = tasks ? json_array_get_count(tasks) : 0;

    size_t off = 0;
    int found = 0;
    for (size_t i = 0; i < count && off < cap; ++i) {
        JSON_Object* t = json_array_get_object(tasks, i);
        const char* text = json_object_get_string(t, "text");
        if (!text || !strstr(text, query)) continue;
        long id = (long)json_object_get_number(t, "id");
        int done = json_object_get_boolean(t, "done") > 0;
        off += (size_t)snprintf(out + off, cap - off, "%s #%ld %s\n",
                                done ? "\xe2\x98\x91" : "\xe2\x98\x90", id, text);
        ++found;
    }
    if (!found) snprintf(out, cap, "По запросу «%s» ничего не найдено.", query);
}

void VTL_bot_store_ListByStatus(VTL_bot_Store* store, const char* chat_id,
                                int done_flag, char* out, size_t cap)
{
    if (!out || cap == 0) return;
    out[0] = '\0';
    if (!store || !chat_id) return;

    JSON_Object* chat = VTL_bot_store_Chat(store, chat_id);
    JSON_Array* tasks = chat ? json_object_get_array(chat, "tasks") : NULL;
    size_t count = tasks ? json_array_get_count(tasks) : 0;

    size_t off = (size_t)snprintf(out, cap, "%s:\n",
                                  done_flag ? "Выполненные" : "Невыполненные");
    int shown = 0;
    for (size_t i = 0; i < count && off < cap; ++i) {
        JSON_Object* t = json_array_get_object(tasks, i);
        int done = json_object_get_boolean(t, "done") > 0;
        if (done != (done_flag ? 1 : 0)) continue;
        long id = (long)json_object_get_number(t, "id");
        const char* text = json_object_get_string(t, "text");
        off += (size_t)snprintf(out + off, cap - off, "#%ld %s\n", id, text ? text : "");
        ++shown;
    }
    if (!shown) snprintf(out, cap, "%s задач нет.",
                         done_flag ? "Выполненных" : "Невыполненных");
}

int VTL_bot_store_MarkUndone(VTL_bot_Store* store, const char* chat_id, long id)
{
    if (!store || !chat_id) return 0;
    JSON_Object* chat = VTL_bot_store_Chat(store, chat_id);
    JSON_Array* tasks = chat ? json_object_get_array(chat, "tasks") : NULL;
    int idx = VTL_bot_store_FindTask(tasks, id);
    if (idx < 0) return 0;

    json_object_set_boolean(json_array_get_object(tasks, (size_t)idx), "done", 0);
    store->dirty = 1;
    VTL_bot_store_Save(store);
    return 1;
}

int VTL_bot_store_ClearAll(VTL_bot_Store* store, const char* chat_id)
{
    if (!store || !chat_id) return 0;
    JSON_Object* chat = VTL_bot_store_Chat(store, chat_id);
    JSON_Array* tasks = chat ? json_object_get_array(chat, "tasks") : NULL;
    size_t count = tasks ? json_array_get_count(tasks) : 0;
    for (size_t i = count; i > 0; --i) json_array_remove(tasks, i - 1);
    if (count) { store->dirty = 1; VTL_bot_store_Save(store); }
    return (int)count;
}

int VTL_bot_store_SetPriority(VTL_bot_Store* store, const char* chat_id,
                              long id, int prio)
{
    if (!store || !chat_id) return 0;
    JSON_Object* chat = VTL_bot_store_Chat(store, chat_id);
    JSON_Array* tasks = chat ? json_object_get_array(chat, "tasks") : NULL;
    int idx = VTL_bot_store_FindTask(tasks, id);
    if (idx < 0) return 0;

    json_object_set_number(json_array_get_object(tasks, (size_t)idx),
                           "prio", (double)prio);
    store->dirty = 1;
    VTL_bot_store_Save(store);
    return 1;
}

int VTL_bot_store_SetTag(VTL_bot_Store* store, const char* chat_id,
                         long id, const char* tag)
{
    if (!store || !chat_id || !tag) return 0;
    JSON_Object* chat = VTL_bot_store_Chat(store, chat_id);
    JSON_Array* tasks = chat ? json_object_get_array(chat, "tasks") : NULL;
    int idx = VTL_bot_store_FindTask(tasks, id);
    if (idx < 0) return 0;

    json_object_set_string(json_array_get_object(tasks, (size_t)idx), "tag", tag);
    store->dirty = 1;
    VTL_bot_store_Save(store);
    return 1;
}

void VTL_bot_store_FormatEdgeTask(VTL_bot_Store* store, const char* chat_id,
                                  int want_last, char* out, size_t cap)
{
    if (!out || cap == 0) return;
    out[0] = '\0';
    if (!store || !chat_id) return;

    JSON_Object* chat = VTL_bot_store_Chat(store, chat_id);
    JSON_Array* tasks = chat ? json_object_get_array(chat, "tasks") : NULL;
    size_t count = tasks ? json_array_get_count(tasks) : 0;
    if (count == 0) { snprintf(out, cap, "Список задач пуст."); return; }

    JSON_Object* t = json_array_get_object(tasks, want_last ? count - 1 : 0);
    long id = (long)json_object_get_number(t, "id");
    const char* text = json_object_get_string(t, "text");
    snprintf(out, cap, "#%ld %s", id, text ? text : "");
}

void VTL_bot_store_ExportTasks(VTL_bot_Store* store, const char* chat_id,
                               char* out, size_t cap)
{
    if (!out || cap == 0) return;
    out[0] = '\0';
    if (!store || !chat_id) return;

    JSON_Object* chat = VTL_bot_store_Chat(store, chat_id);
    JSON_Array* tasks = chat ? json_object_get_array(chat, "tasks") : NULL;
    size_t count = tasks ? json_array_get_count(tasks) : 0;
    if (count == 0) { snprintf(out, cap, "Нет задач для экспорта."); return; }

    size_t off = 0;
    for (size_t i = 0; i < count && off < cap; ++i) {
        JSON_Object* t = json_array_get_object(tasks, i);
        const char* text = json_object_get_string(t, "text");
        int done = json_object_get_boolean(t, "done") > 0;
        off += (size_t)snprintf(out + off, cap - off, "%lu. %s [%s]\n",
                                (unsigned long)(i + 1), text ? text : "",
                                done ? "x" : " ");
    }
}

long VTL_bot_store_AddReminder(VTL_bot_Store* store, const char* chat_id,
                               time_t due, const char* text)
{
    if (!store || !chat_id || !text) return -1;
    JSON_Object* chat = VTL_bot_store_Chat(store, chat_id);
    if (!chat) return -1;

    long id = (long)json_object_get_number(chat, "next_reminder_id");
    if (id <= 0) id = 1;

    JSON_Array* rem = json_object_get_array(chat, "reminders");
    JSON_Value* rv = json_value_init_object();
    JSON_Object* ro = json_value_get_object(rv);
    json_object_set_number(ro, "id", (double)id);
    json_object_set_string(ro, "text", text);
    json_object_set_number(ro, "due", (double)due);
    json_object_set_boolean(ro, "fired", 0);
    json_array_append_value(rem, rv);

    json_object_set_number(chat, "next_reminder_id", (double)(id + 1));
    store->dirty = 1;
    VTL_bot_store_Save(store);
    return id;
}

/* Ищет индекс напоминания по id; -1 если нет. */
static int VTL_bot_store_FindReminder(JSON_Array* rem, long id)
{
    size_t count = rem ? json_array_get_count(rem) : 0;
    for (size_t i = 0; i < count; ++i) {
        if ((long)json_object_get_number(json_array_get_object(rem, i), "id") == id)
            return (int)i;
    }
    return -1;
}

void VTL_bot_store_ListReminders(VTL_bot_Store* store, const char* chat_id,
                                 char* out, size_t cap)
{
    if (!out || cap == 0) return;
    out[0] = '\0';
    if (!store || !chat_id) return;

    JSON_Object* chat = VTL_bot_store_Chat(store, chat_id);
    JSON_Array* rem = chat ? json_object_get_array(chat, "reminders") : NULL;
    size_t count = rem ? json_array_get_count(rem) : 0;

    size_t off = (size_t)snprintf(out, cap, "Напоминания:\n");
    int shown = 0;
    for (size_t i = 0; i < count && off < cap; ++i) {
        JSON_Object* r = json_array_get_object(rem, i);
        if (json_object_get_boolean(r, "fired") > 0) continue;
        long id = (long)json_object_get_number(r, "id");
        time_t due = (time_t)json_object_get_number(r, "due");
        const char* text = json_object_get_string(r, "text");

        char ts[32];
        struct tm tm_due;
#if defined(_WIN32)
        localtime_s(&tm_due, &due);
#else
        localtime_r(&due, &tm_due);
#endif
        strftime(ts, sizeof(ts), "%d.%m %H:%M", &tm_due);
        off += (size_t)snprintf(out + off, cap - off, "#%ld %s — %s\n",
                                id, ts, text ? text : "");
        ++shown;
    }
    if (!shown) snprintf(out, cap, "Активных напоминаний нет.");
}

int VTL_bot_store_CancelReminder(VTL_bot_Store* store, const char* chat_id,
                                 long id)
{
    if (!store || !chat_id) return 0;
    JSON_Object* chat = VTL_bot_store_Chat(store, chat_id);
    JSON_Array* rem = chat ? json_object_get_array(chat, "reminders") : NULL;
    int idx = VTL_bot_store_FindReminder(rem, id);
    if (idx < 0) return 0;

    json_array_remove(rem, (size_t)idx);
    store->dirty = 1;
    VTL_bot_store_Save(store);
    return 1;
}

int VTL_bot_store_ClearReminders(VTL_bot_Store* store, const char* chat_id)
{
    if (!store || !chat_id) return 0;
    JSON_Object* chat = VTL_bot_store_Chat(store, chat_id);
    JSON_Array* rem = chat ? json_object_get_array(chat, "reminders") : NULL;
    size_t count = rem ? json_array_get_count(rem) : 0;
    for (size_t i = count; i > 0; --i) json_array_remove(rem, i - 1);
    if (count) { store->dirty = 1; VTL_bot_store_Save(store); }
    return (int)count;
}

void VTL_bot_store_NextReminder(VTL_bot_Store* store, const char* chat_id,
                                char* out, size_t cap)
{
    if (!out || cap == 0) return;
    out[0] = '\0';
    if (!store || !chat_id) return;

    JSON_Object* chat = VTL_bot_store_Chat(store, chat_id);
    JSON_Array* rem = chat ? json_object_get_array(chat, "reminders") : NULL;
    size_t count = rem ? json_array_get_count(rem) : 0;

    time_t      best = 0;
    const char* best_text = NULL;
    long        best_id = 0;
    int         found = 0;
    for (size_t i = 0; i < count; ++i) {
        JSON_Object* r = json_array_get_object(rem, i);
        if (json_object_get_boolean(r, "fired") > 0) continue;
        time_t due = (time_t)json_object_get_number(r, "due");
        if (!found || due < best) {
            best = due;
            best_text = json_object_get_string(r, "text");
            best_id = (long)json_object_get_number(r, "id");
            found = 1;
        }
    }
    if (!found) { snprintf(out, cap, "Активных напоминаний нет."); return; }

    char ts[32];
    struct tm tm_due;
#if defined(_WIN32)
    localtime_s(&tm_due, &best);
#else
    localtime_r(&best, &tm_due);
#endif
    strftime(ts, sizeof(ts), "%d.%m %H:%M", &tm_due);
    snprintf(out, cap, "Ближайшее: #%ld %s — %s",
             best_id, ts, best_text ? best_text : "");
}

/* Возвращает массив "notes" чата, создавая его при отсутствии. */
static JSON_Array* VTL_bot_store_Notes(VTL_bot_Store* store, const char* chat_id)
{
    JSON_Object* chat = VTL_bot_store_Chat(store, chat_id);
    if (!chat) return NULL;
    JSON_Array* notes = json_object_get_array(chat, "notes");
    if (!notes) {
        json_object_set_value(chat, "notes", json_value_init_array());
        notes = json_object_get_array(chat, "notes");
    }
    return notes;
}

long VTL_bot_store_AddNote(VTL_bot_Store* store, const char* chat_id,
                           const char* text)
{
    if (!store || !chat_id || !text) return -1;
    JSON_Object* chat = VTL_bot_store_Chat(store, chat_id);
    JSON_Array* notes = VTL_bot_store_Notes(store, chat_id);
    if (!chat || !notes) return -1;

    long id = (long)json_object_get_number(chat, "next_note_id");
    if (id <= 0) id = 1;

    JSON_Value* nv = json_value_init_object();
    JSON_Object* no = json_value_get_object(nv);
    json_object_set_number(no, "id", (double)id);
    json_object_set_string(no, "text", text);
    json_array_append_value(notes, nv);

    json_object_set_number(chat, "next_note_id", (double)(id + 1));
    store->dirty = 1;
    VTL_bot_store_Save(store);
    return id;
}

void VTL_bot_store_ListNotes(VTL_bot_Store* store, const char* chat_id,
                             char* out, size_t cap)
{
    if (!out || cap == 0) return;
    out[0] = '\0';
    if (!store || !chat_id) return;

    JSON_Array* notes = VTL_bot_store_Notes(store, chat_id);
    size_t count = notes ? json_array_get_count(notes) : 0;
    if (count == 0) {
        snprintf(out, cap, "Заметок нет. Добавь: /note <текст>");
        return;
    }

    size_t off = (size_t)snprintf(out, cap, "Заметки:\n");
    for (size_t i = 0; i < count && off < cap; ++i) {
        JSON_Object* n = json_array_get_object(notes, i);
        long id = (long)json_object_get_number(n, "id");
        const char* text = json_object_get_string(n, "text");
        off += (size_t)snprintf(out + off, cap - off, "#%ld %s\n", id, text ? text : "");
    }
}

int VTL_bot_store_DelNote(VTL_bot_Store* store, const char* chat_id, long id)
{
    if (!store || !chat_id) return 0;
    JSON_Array* notes = VTL_bot_store_Notes(store, chat_id);
    size_t count = notes ? json_array_get_count(notes) : 0;
    for (size_t i = 0; i < count; ++i) {
        if ((long)json_object_get_number(json_array_get_object(notes, i), "id") == id) {
            json_array_remove(notes, i);
            store->dirty = 1;
            VTL_bot_store_Save(store);
            return 1;
        }
    }
    return 0;
}

int VTL_bot_store_FireDueReminders(VTL_bot_Store* store, time_t now,
                                   VTL_bot_ReminderFn fn, void* user_data)
{
    if (!store || !fn) return 0;
    JSON_Object* chats = VTL_bot_store_Chats(store);
    size_t chat_count = chats ? json_object_get_count(chats) : 0;

    int fired = 0;
    for (size_t ci = 0; ci < chat_count; ++ci) {
        const char* chat_id = json_object_get_name(chats, ci);
        JSON_Object* chat = json_object_get_object(chats, chat_id);
        JSON_Array* rem = chat ? json_object_get_array(chat, "reminders") : NULL;
        size_t rc = rem ? json_array_get_count(rem) : 0;

        for (size_t i = 0; i < rc; ++i) {
            JSON_Object* r = json_array_get_object(rem, i);
            if (json_object_get_boolean(r, "fired") > 0) continue;
            time_t due = (time_t)json_object_get_number(r, "due");
            if (due > now) continue;

            const char* text = json_object_get_string(r, "text");
            fn(chat_id, text ? text : "", user_data);
            json_object_set_boolean(r, "fired", 1);
            ++fired;
        }
    }
    if (fired) { store->dirty = 1; VTL_bot_store_Save(store); }
    return fired;
}
