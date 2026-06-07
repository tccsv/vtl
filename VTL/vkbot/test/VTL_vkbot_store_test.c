/* Тесты store-логики VK-бота: операции (CRUD), баланс, категории, лимит.
 * Работают с временным JSON-файлом, без сети и VK. */
#include <VTL/vkbot/store/VTL_vkbot_store.h>
#include <VTL/test/VTL_test_data.h>

#include <stdio.h>
#include <string.h>
#include <time.h>

#define STORE_PATH "_vkbot_test_store.json"
#define PEER       "2000000001"

static VTL_vkbot_Store* fresh_store(void)
{
    remove(STORE_PATH);
    return VTL_vkbot_store_Open(STORE_PATH);
}

TEST(add_assigns_incrementing_ids)
{
    VTL_vkbot_Store* s = fresh_store();
    VTL_ASSERT(s != NULL, "store opens");
    VTL_ASSERT(VTL_vkbot_store_AddOp(s, PEER, 100.0, 0, "кофе") == 1, "first id is 1");
    VTL_ASSERT(VTL_vkbot_store_AddOp(s, PEER, 500.0, 1, "зарплата") == 2, "second id is 2");
    VTL_vkbot_store_Close(s);
}

TEST(list_shows_amount_and_desc)
{
    VTL_vkbot_Store* s = fresh_store();
    VTL_vkbot_store_AddOp(s, PEER, 100.0, 0, "кофе");

    char out[256];
    VTL_vkbot_store_ListOps(s, PEER, out, sizeof(out));
    VTL_ASSERT(strstr(out, "кофе") != NULL, "list contains desc");
    VTL_ASSERT(strstr(out, "#1") != NULL, "list contains op id");
    VTL_ASSERT(strstr(out, "100.00") != NULL, "list contains amount");
    VTL_vkbot_store_Close(s);
}

TEST(balance_is_income_minus_expense)
{
    VTL_vkbot_Store* s = fresh_store();
    VTL_vkbot_store_AddOp(s, PEER, 1000.0, 1, "доход");
    VTL_vkbot_store_AddOp(s, PEER, 250.0, 0, "расход");

    char out[256];
    VTL_vkbot_store_FormatBalance(s, PEER, out, sizeof(out));
    VTL_ASSERT(strstr(out, "750.00") != NULL, "balance is 1000 - 250 = 750");
    VTL_vkbot_store_Close(s);
}

TEST(edit_amount_only_for_existing_id)
{
    VTL_vkbot_Store* s = fresh_store();
    VTL_vkbot_store_AddOp(s, PEER, 100.0, 0, "op");
    VTL_ASSERT(VTL_vkbot_store_EditOpAmount(s, PEER, 1, 200.0) == 1, "existing op edited");
    VTL_ASSERT(VTL_vkbot_store_EditOpAmount(s, PEER, 99, 5.0) == 0, "missing id ignored");

    char out[256];
    VTL_vkbot_store_FormatBalance(s, PEER, out, sizeof(out));
    VTL_ASSERT(strstr(out, "-200.00") != NULL, "edited amount affects balance");
    VTL_vkbot_store_Close(s);
}

TEST(del_removes_op)
{
    VTL_vkbot_Store* s = fresh_store();
    VTL_vkbot_store_AddOp(s, PEER, 100.0, 0, "op");
    VTL_ASSERT(VTL_vkbot_store_DelOp(s, PEER, 1) == 1, "op deleted");
    VTL_ASSERT(VTL_vkbot_store_DelOp(s, PEER, 1) == 0, "second delete is a no-op");
    VTL_vkbot_store_Close(s);
}

TEST(category_filter_lists_only_matching)
{
    VTL_vkbot_Store* s = fresh_store();
    VTL_vkbot_store_AddOp(s, PEER, 100.0, 0, "такси");
    VTL_vkbot_store_AddOp(s, PEER, 50.0, 0, "метро");
    VTL_ASSERT(VTL_vkbot_store_SetOpCat(s, PEER, 1, "транспорт") == 1, "cat set");

    char out[256];
    VTL_vkbot_store_ListByCat(s, PEER, "транспорт", out, sizeof(out));
    VTL_ASSERT(strstr(out, "такси") != NULL, "matching op shown");
    VTL_ASSERT(strstr(out, "метро") == NULL, "non-matching op hidden");
    VTL_vkbot_store_Close(s);
}

TEST(budget_left_subtracts_expenses)
{
    VTL_vkbot_Store* s = fresh_store();
    VTL_vkbot_store_SetBudget(s, PEER, 1000.0);
    VTL_vkbot_store_AddOp(s, PEER, 300.0, 0, "расход");

    char out[256];
    VTL_vkbot_store_FormatBudgetLeft(s, PEER, out, sizeof(out));
    VTL_ASSERT(strstr(out, "700.00") != NULL, "left is 1000 - 300 = 700");
    VTL_vkbot_store_Close(s);
}

TEST(ops_persist_after_reopen)
{
    VTL_vkbot_Store* s = fresh_store();
    VTL_vkbot_store_AddOp(s, PEER, 42.0, 0, "persist me");
    VTL_vkbot_store_Close(s);

    s = VTL_vkbot_store_Open(STORE_PATH);
    char out[256];
    VTL_vkbot_store_ListOps(s, PEER, out, sizeof(out));
    VTL_ASSERT(strstr(out, "persist me") != NULL, "op survives reopen");
    VTL_vkbot_store_Close(s);
}

int main(void)
{
    VTL_RUN_TEST(add_assigns_incrementing_ids);
    VTL_RUN_TEST(list_shows_amount_and_desc);
    VTL_RUN_TEST(balance_is_income_minus_expense);
    VTL_RUN_TEST(edit_amount_only_for_existing_id);
    VTL_RUN_TEST(del_removes_op);
    VTL_RUN_TEST(category_filter_lists_only_matching);
    VTL_RUN_TEST(budget_left_subtracts_expenses);
    VTL_RUN_TEST(ops_persist_after_reopen);
    remove(STORE_PATH);
    return VTL_test_result();
}
