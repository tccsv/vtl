#ifndef _VTL_SCHEDULER_REPO_H
#define _VTL_SCHEDULER_REPO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <VTL/scheduler/VTL_scheduler_data.h>
#include <VTL/utils/db/VTL_db_credentals.h>
#include <VTL/VTL_app_result.h>
#include <libpq-fe.h>
#include <time.h>

/* ------------------------------------------------------------------ */
/* Соединение (обёртка над PGconn, владеет ресурсом)                  */
/* ------------------------------------------------------------------ */
typedef struct _VTL_scheduler_Repo
{
    PGconn* conn;
} VTL_scheduler_Repo;

/* Открыть соединение к БД.
 * Возвращает VTL_res_kOk при успехе; заполняет repo->conn.           */
VTL_AppResult VTL_scheduler_repo_Open(VTL_scheduler_Repo*       repo,
                                        VTL_db_Credentals*  creds);

/* Закрыть соединение и обнулить указатель.                            */
void          VTL_scheduler_repo_Close(VTL_scheduler_Repo* repo);

/* DDL: создаёт таблицу scheduled_posts если её нет.                   */
VTL_AppResult VTL_scheduler_repo_EnsureTable(VTL_scheduler_Repo* repo);

/* ------------------------------------------------------------------ */
/* CRUD                                                                 */
/* ------------------------------------------------------------------ */

/* CREATE: вставить новую запись; заполняет post->id значением из БД.  */
VTL_AppResult VTL_scheduler_repo_Insert(VTL_scheduler_Repo*       repo,
                                         VTL_scheduler_Post*       post);

/* READ (single): загрузить запись по первичному ключу.
 * Вызывающий обязан позвонить VTL_scheduler_post_Free(out) потом.     */
VTL_AppResult VTL_scheduler_repo_GetById(VTL_scheduler_Repo*  repo,
                                          long long            id,
                                          VTL_scheduler_Post*  out);

/* READ (pending): получить записи, у которых
 *   send_date_time <= now+lookahead_sec  AND  executed = false
 * Результат складывается в *list (уже инициализированный пустой).
 * Вызывающий обязан позвонить VTL_scheduler_postlist_Free(list) потом.*/
VTL_AppResult VTL_scheduler_repo_GetPending(VTL_scheduler_Repo*    repo,
                                              int                    lookahead_sec,
                                              VTL_scheduler_PostList* list);

/* UPDATE: обновить все поля записи по id.                             */
VTL_AppResult VTL_scheduler_repo_Update(VTL_scheduler_Repo*       repo,
                                         const VTL_scheduler_Post* post);

/* UPDATE (executed flag): пометить запись отработанной.               */
VTL_AppResult VTL_scheduler_repo_MarkExecuted(VTL_scheduler_Repo* repo,
                                               long long           id);

/* DELETE: удалить запись по id.                                        */
VTL_AppResult VTL_scheduler_repo_Delete(VTL_scheduler_Repo* repo,
                                         long long           id);

/* ------------------------------------------------------------------ */
/* Память                                                               */
/* ------------------------------------------------------------------ */

/* Освободить строки внутри одной записи (не саму структуру).          */
void VTL_scheduler_post_Free(VTL_scheduler_Post* post);

/* Освободить все записи в списке и сам массив items.                  */
void VTL_scheduler_postlist_Free(VTL_scheduler_PostList* list);

#ifdef __cplusplus
}
#endif

#endif /* _VTL_SCHEDULER_REPO_H */
