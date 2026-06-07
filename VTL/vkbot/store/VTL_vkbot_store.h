#ifndef _VTL_VKBOT_STORE_H
#define _VTL_VKBOT_STORE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/VTL_app_result.h>
#include <stddef.h>
#include <time.h>

/*
 * Хранилище личных финансов в JSON-файле (parson).
 *
 * Модель файла:
 *   {
 *     "peers": {
 *       "<peer_id>": {
 *         "next_op_id":  <число>,
 *         "next_cat_id": <число>,
 *         "budget":      <число>,   // месячный лимит расходов, 0 = не задан
 *         "ops":  [ {"id","amount","kind","desc","cat","created"}, ... ],
 *         "cats": [ {"id","name"}, ... ]
 *       }
 *     }
 *   }
 *
 * kind: 0 — расход, 1 — доход. Данные разделены по peer_id (свой кошелёк
 * у каждого диалога VK).
 */

typedef struct VTL_vkbot_Store VTL_vkbot_Store;

/* Открывает (или создаёт пустое) хранилище из файла path. NULL — ошибка. */
VTL_vkbot_Store* VTL_vkbot_store_Open(const char* path);

/* Сохраняет (если были изменения) и освобождает хранилище. */
void VTL_vkbot_store_Close(VTL_vkbot_Store* store);

/* Принудительно сериализует хранилище в файл. */
VTL_AppResult VTL_vkbot_store_Save(VTL_vkbot_Store* store);

/* --- Операции (расходы / доходы) --- */

/* Добавляет операцию (kind: 0 — расход, 1 — доход). id (>0) либо -1. */
long VTL_vkbot_store_AddOp(VTL_vkbot_Store* store, const char* peer,
                           double amount, int kind, const char* desc);

/* Печатает все операции в out (cap байт). */
void VTL_vkbot_store_ListOps(VTL_vkbot_Store* store, const char* peer,
                             char* out, size_t cap);

/* Печатает операции заданного вида (kind: 0 — расходы, 1 — доходы). */
void VTL_vkbot_store_ListByKind(VTL_vkbot_Store* store, const char* peer,
                                int kind, char* out, size_t cap);

/* Меняет сумму операции id. 1 — найдена, 0 — нет. */
int VTL_vkbot_store_EditOpAmount(VTL_vkbot_Store* store, const char* peer,
                                 long id, double amount);

/* Меняет описание операции id. 1 — найдена, 0 — нет. */
int VTL_vkbot_store_EditOpDesc(VTL_vkbot_Store* store, const char* peer,
                               long id, const char* desc);

/* Удаляет операцию id. 1 — удалена, 0 — не найдена. */
int VTL_vkbot_store_DelOp(VTL_vkbot_Store* store, const char* peer, long id);

/* Удаляет все операции. Возвращает число удалённых. */
int VTL_vkbot_store_ClearOps(VTL_vkbot_Store* store, const char* peer);

/* Печатает операции, чьё описание содержит query. */
void VTL_vkbot_store_SearchOps(VTL_vkbot_Store* store, const char* peer,
                               const char* query, char* out, size_t cap);

/* Печатает первую (want_last=0) или последнюю (want_last=1) операцию. */
void VTL_vkbot_store_FormatEdgeOp(VTL_vkbot_Store* store, const char* peer,
                                  int want_last, char* out, size_t cap);

/* Возвращает число операций. */
int VTL_vkbot_store_CountOps(VTL_vkbot_Store* store, const char* peer);

/* Печатает сводку: доход / расход / баланс / число операций. */
void VTL_vkbot_store_Stats(VTL_vkbot_Store* store, const char* peer,
                           char* out, size_t cap);

/* Выгружает операции простым нумерованным списком. */
void VTL_vkbot_store_ExportOps(VTL_vkbot_Store* store, const char* peer,
                               char* out, size_t cap);

/* Печатает операции за сегодня (относительно now). */
void VTL_vkbot_store_ListToday(VTL_vkbot_Store* store, const char* peer,
                               time_t now, char* out, size_t cap);

/* Печатает крупнейший расход. */
void VTL_vkbot_store_FormatMaxExpense(VTL_vkbot_Store* store, const char* peer,
                                      char* out, size_t cap);

/* Печатает средний расход. */
void VTL_vkbot_store_FormatAvgExpense(VTL_vkbot_Store* store, const char* peer,
                                      char* out, size_t cap);

/* Печатает текущий баланс (доходы − расходы). */
void VTL_vkbot_store_FormatBalance(VTL_vkbot_Store* store, const char* peer,
                                   char* out, size_t cap);

/* Присваивает категорию операции id. 1 — найдена, 0 — нет. */
int VTL_vkbot_store_SetOpCat(VTL_vkbot_Store* store, const char* peer,
                             long id, const char* cat);

/* Печатает операции с заданной категорией. */
void VTL_vkbot_store_ListByCat(VTL_vkbot_Store* store, const char* peer,
                               const char* cat, char* out, size_t cap);

/* --- Категории --- */

/* Добавляет категорию. id (>0) либо -1. */
long VTL_vkbot_store_AddCat(VTL_vkbot_Store* store, const char* peer,
                            const char* name);

/* Печатает список категорий. */
void VTL_vkbot_store_ListCats(VTL_vkbot_Store* store, const char* peer,
                              char* out, size_t cap);

/* Удаляет категорию id. 1 — найдена, 0 — нет. */
int VTL_vkbot_store_DelCat(VTL_vkbot_Store* store, const char* peer, long id);

/* --- Лимит расходов --- */

/* Устанавливает месячный лимит расходов. */
void VTL_vkbot_store_SetBudget(VTL_vkbot_Store* store, const char* peer,
                               double amount);

/* Печатает остаток лимита (лимит − суммарные расходы). */
void VTL_vkbot_store_FormatBudgetLeft(VTL_vkbot_Store* store, const char* peer,
                                      char* out, size_t cap);

#ifdef __cplusplus
}
#endif

#endif /* _VTL_VKBOT_STORE_H */
