#ifndef _VTL_SCHEDULER_DISPATCHER_H
#define _VTL_SCHEDULER_DISPATCHER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <VTL/scheduler/VTL_scheduler_data.h>
#include <VTL/scheduler/db/VTL_scheduler_repo.h>
#include <VTL/VTL_app_result.h>

/* ------------------------------------------------------------------ */
/* Отправить одну запись в нужную соц-сеть.
 * Функция десериализует metadata, определяет content_type и вызывает
 * соответствующий метод платформы.
 * Используется и синхронно, и как тело потока.                        */
VTL_AppResult VTL_scheduler_dispatcher_Send(VTL_scheduler_Repo*       repo,
                                              const VTL_scheduler_Post* post);

/* ------------------------------------------------------------------ */
/* Главный цикл планировщика.
 *
 * Каждые poll_interval_sec секунд вычитывает из БД все записи
 * с send_date_time <= NOW() + lookahead_sec, которые ещё не отправлены,
 * и параллельно отправляет их (по одному pthread на запись).
 * После успешной отправки помечает запись executed = true.
 *
 * Функция блокирует поток. Для остановки установите *stop_flag = 1
 * из другого потока (volatile int).                                   */
void VTL_scheduler_dispatcher_Run(VTL_scheduler_Repo* repo,
                                   int                 poll_interval_sec,
                                   int                 lookahead_sec,
                                   volatile int*       stop_flag);

#ifdef __cplusplus
}
#endif

#endif /* _VTL_SCHEDULER_DISPATCHER_H */
