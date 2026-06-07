/* Тесты store-логики бота: задачи (CRUD) и срабатывание напоминаний.
 * Работают с временным JSON-файлом, без сети и Telegram. */
#include <VTL/bot/store/VTL_bot_store.h>
#include <VTL/test/VTL_test_data.h>

#include <stdio.h>
#include <string.h>
#include <time.h>

#define STORE_PATH "_bot_test_store.json"
#define CHAT       "12345"

static VTL_bot_Store* fresh_store(void)
{
    remove(STORE_PATH);
    return VTL_bot_store_Open(STORE_PATH);
}

TEST(add_assigns_incrementing_ids)
{
    VTL_bot_Store* s = fresh_store();
    VTL_ASSERT(s != NULL, "store opens");
    VTL_ASSERT(VTL_bot_store_AddTask(s, CHAT, "buy milk") == 1, "first id is 1");
    VTL_ASSERT(VTL_bot_store_AddTask(s, CHAT, "call mom") == 2, "second id is 2");
    VTL_bot_store_Close(s);
}

TEST(list_shows_text_and_id)
{
    VTL_bot_Store* s = fresh_store();
    VTL_bot_store_AddTask(s, CHAT, "buy milk");

    char out[256];
    VTL_bot_store_ListTasks(s, CHAT, out, sizeof(out));
    VTL_ASSERT(strstr(out, "buy milk") != NULL, "list contains task text");
    VTL_ASSERT(strstr(out, "#1") != NULL, "list contains task id");
    VTL_bot_store_Close(s);
}

TEST(done_only_for_existing_id)
{
    VTL_bot_Store* s = fresh_store();
    VTL_bot_store_AddTask(s, CHAT, "task");
    VTL_ASSERT(VTL_bot_store_MarkDone(s, CHAT, 1) == 1, "existing task marked");
    VTL_ASSERT(VTL_bot_store_MarkDone(s, CHAT, 99) == 0, "missing id ignored");
    VTL_bot_store_Close(s);
}

TEST(del_removes_task)
{
    VTL_bot_Store* s = fresh_store();
    VTL_bot_store_AddTask(s, CHAT, "task");
    VTL_ASSERT(VTL_bot_store_DelTask(s, CHAT, 1) == 1, "task deleted");
    VTL_ASSERT(VTL_bot_store_DelTask(s, CHAT, 1) == 0, "second delete is a no-op");
    VTL_bot_store_Close(s);
}

TEST(clear_removes_only_done)
{
    VTL_bot_Store* s = fresh_store();
    VTL_bot_store_AddTask(s, CHAT, "a");
    VTL_bot_store_AddTask(s, CHAT, "b");
    VTL_bot_store_MarkDone(s, CHAT, 1);
    VTL_ASSERT(VTL_bot_store_ClearDone(s, CHAT) == 1, "one done task cleared");

    char out[256];
    VTL_bot_store_ListTasks(s, CHAT, out, sizeof(out));
    VTL_ASSERT(strstr(out, "#2") != NULL, "undone task stays");
    VTL_bot_store_Close(s);
}

static int g_fired;
static void count_fired(const char* chat_id, const char* text, void* ud)
{
    (void)chat_id; (void)text; (void)ud;
    ++g_fired;
}

TEST(reminders_fire_due_once)
{
    VTL_bot_Store* s = fresh_store();
    time_t now = time(NULL);
    VTL_bot_store_AddReminder(s, CHAT, now - 10, "past");
    VTL_bot_store_AddReminder(s, CHAT, now + 3600, "future");

    g_fired = 0;
    VTL_ASSERT(VTL_bot_store_FireDueReminders(s, now, count_fired, NULL) == 1,
               "only the due reminder fires");
    VTL_ASSERT(g_fired == 1, "callback runs once");
    VTL_ASSERT(VTL_bot_store_FireDueReminders(s, now, count_fired, NULL) == 0,
               "fired reminder does not repeat");
    VTL_bot_store_Close(s);
}

TEST(tasks_persist_after_reopen)
{
    VTL_bot_Store* s = fresh_store();
    VTL_bot_store_AddTask(s, CHAT, "persist me");
    VTL_bot_store_Close(s);

    s = VTL_bot_store_Open(STORE_PATH);
    char out[256];
    VTL_bot_store_ListTasks(s, CHAT, out, sizeof(out));
    VTL_ASSERT(strstr(out, "persist me") != NULL, "task survives reopen");
    VTL_bot_store_Close(s);
}

int main(void)
{
    VTL_RUN_TEST(add_assigns_incrementing_ids);
    VTL_RUN_TEST(list_shows_text_and_id);
    VTL_RUN_TEST(done_only_for_existing_id);
    VTL_RUN_TEST(del_removes_task);
    VTL_RUN_TEST(clear_removes_only_done);
    VTL_RUN_TEST(reminders_fire_due_once);
    VTL_RUN_TEST(tasks_persist_after_reopen);
    remove(STORE_PATH);
    return VTL_test_result();
}
