/* localtime_r — POSIX; при -std=c11 объявление скрыто, явно просим POSIX. */
#if !defined(_WIN32)
#  define _POSIX_C_SOURCE 200809L
#endif

#include <VTL/vkbot/store/VTL_vkbot_store.h>
#include <VTL/vkbot/VTL_vkbot_data.h>

#include <parson/parson.h>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct VTL_vkbot_Store {
    JSON_Value* root;   /* корневой объект всего файла */
    char*       path;   /* путь к файлу хранилища */
    int         dirty;  /* есть несохранённые изменения */
};

/* ============================ доступ к структуре ============================ */

/* Возвращает объект "peers", создавая его при отсутствии. */
static JSON_Object* VTL_vkbot_store_Peers(VTL_vkbot_Store* store)
{
    JSON_Object* root = json_value_get_object(store->root);
    JSON_Object* peers = json_object_get_object(root, "peers");
    if (!peers) {
        json_object_set_value(root, "peers", json_value_init_object());
        peers = json_object_get_object(root, "peers");
    }
    return peers;
}

/* Возвращает объект конкретного диалога, создавая его и поля при отсутствии. */
static JSON_Object* VTL_vkbot_store_Peer(VTL_vkbot_Store* store, const char* peer)
{
    JSON_Object* peers = VTL_vkbot_store_Peers(store);
    if (!peers || !peer) return NULL;

    JSON_Object* p = json_object_get_object(peers, peer);
    if (!p) {
        json_object_set_value(peers, peer, json_value_init_object());
        p = json_object_get_object(peers, peer);
        if (!p) return NULL;
        json_object_set_number(p, "next_op_id", 1);
        json_object_set_number(p, "next_cat_id", 1);
        json_object_set_number(p, "budget", 0);
        json_object_set_value(p, "ops", json_value_init_array());
        json_object_set_value(p, "cats", json_value_init_array());
    }
    return p;
}

/* Ищет индекс операции по id; -1 если нет. */
static int VTL_vkbot_store_FindOp(JSON_Array* ops, long id)
{
    size_t count = ops ? json_array_get_count(ops) : 0;
    for (size_t i = 0; i < count; ++i) {
        if ((long)json_object_get_number(json_array_get_object(ops, i), "id") == id)
            return (int)i;
    }
    return -1;
}

/* Суммирует операции заданного вида (kind: 0 — расходы, 1 — доходы). */
static double VTL_vkbot_store_SumByKind(JSON_Array* ops, int kind)
{
    size_t count = ops ? json_array_get_count(ops) : 0;
    double sum = 0;
    for (size_t i = 0; i < count; ++i) {
        JSON_Object* o = json_array_get_object(ops, i);
        if ((int)json_object_get_number(o, "kind") == kind)
            sum += json_object_get_number(o, "amount");
    }
    return sum;
}

/* Дописывает одну операцию строкой "#id ±сумма описание [категория]". */
static void VTL_vkbot_store_AppendOp(JSON_Array* ops, size_t i,
                                     char* out, size_t cap, size_t* off)
{
    if (*off >= cap) return;
    JSON_Object* o = json_array_get_object(ops, i);
    long   id     = (long)json_object_get_number(o, "id");
    int    kind   = (int)json_object_get_number(o, "kind");
    double amount = json_object_get_number(o, "amount");
    const char* desc = json_object_get_string(o, "desc");
    const char* cat  = json_object_get_string(o, "cat");

    char catbuf[80];
    if (cat && *cat) snprintf(catbuf, sizeof(catbuf), " [%s]", cat);
    else             catbuf[0] = '\0';

    *off += (size_t)snprintf(out + *off, cap - *off, "#%ld %c%.2f %s%s\n",
                             id, kind ? '+' : '-', amount,
                             desc ? desc : "", catbuf);
}

/* ============================ жизненный цикл ============================ */

VTL_vkbot_Store* VTL_vkbot_store_Open(const char* path)
{
    if (!path) return NULL;

    VTL_vkbot_Store* store = (VTL_vkbot_Store*)calloc(1, sizeof(*store));
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

VTL_AppResult VTL_vkbot_store_Save(VTL_vkbot_Store* store)
{
    if (!store) return VTL_res_kInvalidParamErr;
    JSON_Status st = json_serialize_to_file_pretty(store->root, store->path);
    if (st != JSONSuccess) return VTL_res_kFileWriteErr;
    store->dirty = 0;
    return VTL_res_kOk;
}

void VTL_vkbot_store_Close(VTL_vkbot_Store* store)
{
    if (!store) return;
    if (store->dirty) VTL_vkbot_store_Save(store);
    if (store->root) json_value_free(store->root);
    free(store->path);
    free(store);
}

/* ============================ операции ============================ */

long VTL_vkbot_store_AddOp(VTL_vkbot_Store* store, const char* peer,
                           double amount, int kind, const char* desc)
{
    if (!store || !peer) return -1;
    JSON_Object* p = VTL_vkbot_store_Peer(store, peer);
    if (!p) return -1;

    long id = (long)json_object_get_number(p, "next_op_id");
    if (id <= 0) id = 1;

    JSON_Array* ops = json_object_get_array(p, "ops");
    JSON_Value* ov = json_value_init_object();
    JSON_Object* oo = json_value_get_object(ov);
    json_object_set_number(oo, "id", (double)id);
    json_object_set_number(oo, "amount", amount);
    json_object_set_number(oo, "kind", kind ? 1 : 0);
    json_object_set_string(oo, "desc", desc ? desc : "");
    json_object_set_string(oo, "cat", "");
    json_object_set_number(oo, "created", (double)time(NULL));
    json_array_append_value(ops, ov);

    json_object_set_number(p, "next_op_id", (double)(id + 1));
    store->dirty = 1;
    VTL_vkbot_store_Save(store);
    return id;
}

void VTL_vkbot_store_ListOps(VTL_vkbot_Store* store, const char* peer,
                             char* out, size_t cap)
{
    if (!out || cap == 0) return;
    out[0] = '\0';
    if (!store || !peer) return;

    JSON_Object* p = VTL_vkbot_store_Peer(store, peer);
    JSON_Array* ops = p ? json_object_get_array(p, "ops") : NULL;
    size_t count = ops ? json_array_get_count(ops) : 0;

    if (count == 0) {
        snprintf(out, cap, "Операций пока нет. Добавь: /spend <сумма> <описание>");
        return;
    }

    size_t off = (size_t)snprintf(out, cap, "Твои операции:\n");
    for (size_t i = 0; i < count && off < cap; ++i)
        VTL_vkbot_store_AppendOp(ops, i, out, cap, &off);
}

void VTL_vkbot_store_ListByKind(VTL_vkbot_Store* store, const char* peer,
                                int kind, char* out, size_t cap)
{
    if (!out || cap == 0) return;
    out[0] = '\0';
    if (!store || !peer) return;

    JSON_Object* p = VTL_vkbot_store_Peer(store, peer);
    JSON_Array* ops = p ? json_object_get_array(p, "ops") : NULL;
    size_t count = ops ? json_array_get_count(ops) : 0;

    size_t off = (size_t)snprintf(out, cap, "%s:\n",
                                  kind ? "Доходы" : "Расходы");
    int shown = 0;
    for (size_t i = 0; i < count && off < cap; ++i) {
        JSON_Object* o = json_array_get_object(ops, i);
        if ((int)json_object_get_number(o, "kind") != (kind ? 1 : 0)) continue;
        VTL_vkbot_store_AppendOp(ops, i, out, cap, &off);
        ++shown;
    }
    if (!shown) snprintf(out, cap, "%s нет.", kind ? "Доходов" : "Расходов");
}

int VTL_vkbot_store_EditOpAmount(VTL_vkbot_Store* store, const char* peer,
                                 long id, double amount)
{
    if (!store || !peer) return 0;
    JSON_Object* p = VTL_vkbot_store_Peer(store, peer);
    JSON_Array* ops = p ? json_object_get_array(p, "ops") : NULL;
    int idx = VTL_vkbot_store_FindOp(ops, id);
    if (idx < 0) return 0;

    json_object_set_number(json_array_get_object(ops, (size_t)idx),
                           "amount", amount);
    store->dirty = 1;
    VTL_vkbot_store_Save(store);
    return 1;
}

int VTL_vkbot_store_EditOpDesc(VTL_vkbot_Store* store, const char* peer,
                               long id, const char* desc)
{
    if (!store || !peer || !desc) return 0;
    JSON_Object* p = VTL_vkbot_store_Peer(store, peer);
    JSON_Array* ops = p ? json_object_get_array(p, "ops") : NULL;
    int idx = VTL_vkbot_store_FindOp(ops, id);
    if (idx < 0) return 0;

    json_object_set_string(json_array_get_object(ops, (size_t)idx), "desc", desc);
    store->dirty = 1;
    VTL_vkbot_store_Save(store);
    return 1;
}

int VTL_vkbot_store_DelOp(VTL_vkbot_Store* store, const char* peer, long id)
{
    if (!store || !peer) return 0;
    JSON_Object* p = VTL_vkbot_store_Peer(store, peer);
    JSON_Array* ops = p ? json_object_get_array(p, "ops") : NULL;
    int idx = VTL_vkbot_store_FindOp(ops, id);
    if (idx < 0) return 0;

    json_array_remove(ops, (size_t)idx);
    store->dirty = 1;
    VTL_vkbot_store_Save(store);
    return 1;
}

int VTL_vkbot_store_ClearOps(VTL_vkbot_Store* store, const char* peer)
{
    if (!store || !peer) return 0;
    JSON_Object* p = VTL_vkbot_store_Peer(store, peer);
    JSON_Array* ops = p ? json_object_get_array(p, "ops") : NULL;
    size_t count = ops ? json_array_get_count(ops) : 0;
    for (size_t i = count; i > 0; --i) json_array_remove(ops, i - 1);
    if (count) { store->dirty = 1; VTL_vkbot_store_Save(store); }
    return (int)count;
}

void VTL_vkbot_store_SearchOps(VTL_vkbot_Store* store, const char* peer,
                               const char* query, char* out, size_t cap)
{
    if (!out || cap == 0) return;
    out[0] = '\0';
    if (!store || !peer || !query || !*query) return;

    JSON_Object* p = VTL_vkbot_store_Peer(store, peer);
    JSON_Array* ops = p ? json_object_get_array(p, "ops") : NULL;
    size_t count = ops ? json_array_get_count(ops) : 0;

    size_t off = 0;
    int found = 0;
    for (size_t i = 0; i < count && off < cap; ++i) {
        JSON_Object* o = json_array_get_object(ops, i);
        const char* desc = json_object_get_string(o, "desc");
        if (!desc || !strstr(desc, query)) continue;
        VTL_vkbot_store_AppendOp(ops, i, out, cap, &off);
        ++found;
    }
    if (!found) snprintf(out, cap, "По запросу «%s» ничего не найдено.", query);
}

void VTL_vkbot_store_FormatEdgeOp(VTL_vkbot_Store* store, const char* peer,
                                  int want_last, char* out, size_t cap)
{
    if (!out || cap == 0) return;
    out[0] = '\0';
    if (!store || !peer) return;

    JSON_Object* p = VTL_vkbot_store_Peer(store, peer);
    JSON_Array* ops = p ? json_object_get_array(p, "ops") : NULL;
    size_t count = ops ? json_array_get_count(ops) : 0;
    if (count == 0) { snprintf(out, cap, "Операций пока нет."); return; }

    size_t off = 0;
    VTL_vkbot_store_AppendOp(ops, want_last ? count - 1 : 0, out, cap, &off);
}

int VTL_vkbot_store_CountOps(VTL_vkbot_Store* store, const char* peer)
{
    if (!store || !peer) return 0;
    JSON_Object* p = VTL_vkbot_store_Peer(store, peer);
    JSON_Array* ops = p ? json_object_get_array(p, "ops") : NULL;
    return (int)(ops ? json_array_get_count(ops) : 0);
}

void VTL_vkbot_store_Stats(VTL_vkbot_Store* store, const char* peer,
                           char* out, size_t cap)
{
    if (!out || cap == 0) return;
    out[0] = '\0';
    if (!store || !peer) return;

    JSON_Object* p = VTL_vkbot_store_Peer(store, peer);
    JSON_Array* ops = p ? json_object_get_array(p, "ops") : NULL;
    double income  = VTL_vkbot_store_SumByKind(ops, 1);
    double expense = VTL_vkbot_store_SumByKind(ops, 0);
    size_t count = ops ? json_array_get_count(ops) : 0;

    snprintf(out, cap,
             "Доходы:  %.2f\n"
             "Расходы: %.2f\n"
             "Баланс:  %.2f\n"
             "Операций: %lu",
             income, expense, income - expense, (unsigned long)count);
}

void VTL_vkbot_store_ExportOps(VTL_vkbot_Store* store, const char* peer,
                               char* out, size_t cap)
{
    if (!out || cap == 0) return;
    out[0] = '\0';
    if (!store || !peer) return;

    JSON_Object* p = VTL_vkbot_store_Peer(store, peer);
    JSON_Array* ops = p ? json_object_get_array(p, "ops") : NULL;
    size_t count = ops ? json_array_get_count(ops) : 0;
    if (count == 0) { snprintf(out, cap, "Нет операций для экспорта."); return; }

    size_t off = 0;
    for (size_t i = 0; i < count && off < cap; ++i) {
        JSON_Object* o = json_array_get_object(ops, i);
        int    kind   = (int)json_object_get_number(o, "kind");
        double amount = json_object_get_number(o, "amount");
        const char* desc = json_object_get_string(o, "desc");
        off += (size_t)snprintf(out + off, cap - off, "%lu. %c%.2f %s\n",
                                (unsigned long)(i + 1), kind ? '+' : '-',
                                amount, desc ? desc : "");
    }
}

void VTL_vkbot_store_ListToday(VTL_vkbot_Store* store, const char* peer,
                               time_t now, char* out, size_t cap)
{
    if (!out || cap == 0) return;
    out[0] = '\0';
    if (!store || !peer) return;

    /* Полночь сегодняшнего дня по локальному времени. */
    struct tm tm_now;
#if defined(_WIN32)
    localtime_s(&tm_now, &now);
#else
    localtime_r(&now, &tm_now);
#endif
    tm_now.tm_hour = 0;
    tm_now.tm_min  = 0;
    tm_now.tm_sec  = 0;
    time_t day_start = mktime(&tm_now);

    JSON_Object* p = VTL_vkbot_store_Peer(store, peer);
    JSON_Array* ops = p ? json_object_get_array(p, "ops") : NULL;
    size_t count = ops ? json_array_get_count(ops) : 0;

    size_t off = (size_t)snprintf(out, cap, "Операции за сегодня:\n");
    int shown = 0;
    for (size_t i = 0; i < count && off < cap; ++i) {
        JSON_Object* o = json_array_get_object(ops, i);
        time_t created = (time_t)json_object_get_number(o, "created");
        if (created < day_start) continue;
        VTL_vkbot_store_AppendOp(ops, i, out, cap, &off);
        ++shown;
    }
    if (!shown) snprintf(out, cap, "Сегодня операций не было.");
}

void VTL_vkbot_store_FormatMaxExpense(VTL_vkbot_Store* store, const char* peer,
                                      char* out, size_t cap)
{
    if (!out || cap == 0) return;
    out[0] = '\0';
    if (!store || !peer) return;

    JSON_Object* p = VTL_vkbot_store_Peer(store, peer);
    JSON_Array* ops = p ? json_object_get_array(p, "ops") : NULL;
    size_t count = ops ? json_array_get_count(ops) : 0;

    int best = -1;
    double best_amount = 0;
    for (size_t i = 0; i < count; ++i) {
        JSON_Object* o = json_array_get_object(ops, i);
        if ((int)json_object_get_number(o, "kind") != 0) continue;
        double amount = json_object_get_number(o, "amount");
        if (best < 0 || amount > best_amount) { best = (int)i; best_amount = amount; }
    }
    if (best < 0) { snprintf(out, cap, "Расходов пока нет."); return; }

    size_t off = (size_t)snprintf(out, cap, "Крупнейший расход:\n");
    VTL_vkbot_store_AppendOp(ops, (size_t)best, out, cap, &off);
}

void VTL_vkbot_store_FormatAvgExpense(VTL_vkbot_Store* store, const char* peer,
                                      char* out, size_t cap)
{
    if (!out || cap == 0) return;
    out[0] = '\0';
    if (!store || !peer) return;

    JSON_Object* p = VTL_vkbot_store_Peer(store, peer);
    JSON_Array* ops = p ? json_object_get_array(p, "ops") : NULL;
    size_t count = ops ? json_array_get_count(ops) : 0;

    double sum = 0;
    int n = 0;
    for (size_t i = 0; i < count; ++i) {
        JSON_Object* o = json_array_get_object(ops, i);
        if ((int)json_object_get_number(o, "kind") != 0) continue;
        sum += json_object_get_number(o, "amount");
        ++n;
    }
    if (n == 0) { snprintf(out, cap, "Расходов пока нет."); return; }
    snprintf(out, cap, "Средний расход: %.2f (по %d операциям)", sum / n, n);
}

void VTL_vkbot_store_FormatBalance(VTL_vkbot_Store* store, const char* peer,
                                   char* out, size_t cap)
{
    if (!out || cap == 0) return;
    out[0] = '\0';
    if (!store || !peer) return;

    JSON_Object* p = VTL_vkbot_store_Peer(store, peer);
    JSON_Array* ops = p ? json_object_get_array(p, "ops") : NULL;
    double income  = VTL_vkbot_store_SumByKind(ops, 1);
    double expense = VTL_vkbot_store_SumByKind(ops, 0);
    snprintf(out, cap, "Баланс: %.2f", income - expense);
}

int VTL_vkbot_store_SetOpCat(VTL_vkbot_Store* store, const char* peer,
                             long id, const char* cat)
{
    if (!store || !peer || !cat) return 0;
    JSON_Object* p = VTL_vkbot_store_Peer(store, peer);
    JSON_Array* ops = p ? json_object_get_array(p, "ops") : NULL;
    int idx = VTL_vkbot_store_FindOp(ops, id);
    if (idx < 0) return 0;

    json_object_set_string(json_array_get_object(ops, (size_t)idx), "cat", cat);
    store->dirty = 1;
    VTL_vkbot_store_Save(store);
    return 1;
}

void VTL_vkbot_store_ListByCat(VTL_vkbot_Store* store, const char* peer,
                               const char* cat, char* out, size_t cap)
{
    if (!out || cap == 0) return;
    out[0] = '\0';
    if (!store || !peer || !cat || !*cat) return;

    JSON_Object* p = VTL_vkbot_store_Peer(store, peer);
    JSON_Array* ops = p ? json_object_get_array(p, "ops") : NULL;
    size_t count = ops ? json_array_get_count(ops) : 0;

    size_t off = (size_t)snprintf(out, cap, "Категория «%s»:\n", cat);
    int shown = 0;
    for (size_t i = 0; i < count && off < cap; ++i) {
        JSON_Object* o = json_array_get_object(ops, i);
        const char* c = json_object_get_string(o, "cat");
        if (!c || strcmp(c, cat) != 0) continue;
        VTL_vkbot_store_AppendOp(ops, i, out, cap, &off);
        ++shown;
    }
    if (!shown) snprintf(out, cap, "В категории «%s» операций нет.", cat);
}

/* ============================ категории ============================ */

long VTL_vkbot_store_AddCat(VTL_vkbot_Store* store, const char* peer,
                            const char* name)
{
    if (!store || !peer || !name) return -1;
    JSON_Object* p = VTL_vkbot_store_Peer(store, peer);
    if (!p) return -1;

    long id = (long)json_object_get_number(p, "next_cat_id");
    if (id <= 0) id = 1;

    JSON_Array* cats = json_object_get_array(p, "cats");
    JSON_Value* cv = json_value_init_object();
    JSON_Object* co = json_value_get_object(cv);
    json_object_set_number(co, "id", (double)id);
    json_object_set_string(co, "name", name);
    json_array_append_value(cats, cv);

    json_object_set_number(p, "next_cat_id", (double)(id + 1));
    store->dirty = 1;
    VTL_vkbot_store_Save(store);
    return id;
}

void VTL_vkbot_store_ListCats(VTL_vkbot_Store* store, const char* peer,
                              char* out, size_t cap)
{
    if (!out || cap == 0) return;
    out[0] = '\0';
    if (!store || !peer) return;

    JSON_Object* p = VTL_vkbot_store_Peer(store, peer);
    JSON_Array* cats = p ? json_object_get_array(p, "cats") : NULL;
    size_t count = cats ? json_array_get_count(cats) : 0;
    if (count == 0) {
        snprintf(out, cap, "Категорий нет. Добавь: /addcat <название>");
        return;
    }

    size_t off = (size_t)snprintf(out, cap, "Категории:\n");
    for (size_t i = 0; i < count && off < cap; ++i) {
        JSON_Object* c = json_array_get_object(cats, i);
        long id = (long)json_object_get_number(c, "id");
        const char* name = json_object_get_string(c, "name");
        off += (size_t)snprintf(out + off, cap - off, "#%ld %s\n",
                                id, name ? name : "");
    }
}

int VTL_vkbot_store_DelCat(VTL_vkbot_Store* store, const char* peer, long id)
{
    if (!store || !peer) return 0;
    JSON_Object* p = VTL_vkbot_store_Peer(store, peer);
    JSON_Array* cats = p ? json_object_get_array(p, "cats") : NULL;
    size_t count = cats ? json_array_get_count(cats) : 0;
    for (size_t i = 0; i < count; ++i) {
        if ((long)json_object_get_number(json_array_get_object(cats, i), "id") == id) {
            json_array_remove(cats, i);
            store->dirty = 1;
            VTL_vkbot_store_Save(store);
            return 1;
        }
    }
    return 0;
}

/* ============================ лимит расходов ============================ */

void VTL_vkbot_store_SetBudget(VTL_vkbot_Store* store, const char* peer,
                               double amount)
{
    if (!store || !peer) return;
    JSON_Object* p = VTL_vkbot_store_Peer(store, peer);
    if (!p) return;
    json_object_set_number(p, "budget", amount);
    store->dirty = 1;
    VTL_vkbot_store_Save(store);
}

void VTL_vkbot_store_FormatBudgetLeft(VTL_vkbot_Store* store, const char* peer,
                                      char* out, size_t cap)
{
    if (!out || cap == 0) return;
    out[0] = '\0';
    if (!store || !peer) return;

    JSON_Object* p = VTL_vkbot_store_Peer(store, peer);
    double budget = p ? json_object_get_number(p, "budget") : 0;
    if (budget <= 0) {
        snprintf(out, cap, "Лимит расходов не задан. /budget <сумма>");
        return;
    }
    JSON_Array* ops = p ? json_object_get_array(p, "ops") : NULL;
    double expense = VTL_vkbot_store_SumByKind(ops, 0);
    double left = budget - expense;
    snprintf(out, cap, "Лимит: %.2f\nПотрачено: %.2f\nОсталось: %.2f",
             budget, expense, left);
}
