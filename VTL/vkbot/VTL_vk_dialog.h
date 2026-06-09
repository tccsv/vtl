#ifndef VTL_VK_DIALOG_H
#define VTL_VK_DIALOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <VTL/vkbot/VTL_vk_bot.h>

#include <stddef.h>

/* Состояние диалогов и маршрутизация команд. В одном модуле, потому что для
 * VK-бота состояние и разбор тесно связаны: на каждый peer — свой выбор. */

#define VTL_VK_MAX_DIALOGS 64
#define VTL_VK_PATH_MAX    256

/* Выбор оператора для одного диалога (peer). */
typedef struct VTL_vk_Dialog {
    long long                               peer_id;
    VTL_content_platform_flags              targets;   /* площадки (флаги) */
    VTL_publication_marked_text_MarkupType  markup;    /* формат разметки */
    char                                    source[VTL_VK_PATH_MAX];
    int                                     used;
} VTL_vk_Dialog;

/* Куда отправлять ответ: в бою — messages.send, в демо — консоль. */
typedef void (*VTL_vk_EmitFn)(void* ud, long long peer_id, const char* text);

/* Узел обработки: набор диалогов + публикаторы + канал ответа. */
typedef struct VTL_vk_Hub {
    VTL_vk_Dialog            dialogs[VTL_VK_MAX_DIALOGS];
    const VTL_vk_Publishers* pub;
    VTL_vk_EmitFn            emit;
    void*                    emit_ud;
} VTL_vk_Hub;

void VTL_vk_hub_Reset(VTL_vk_Hub* hub, const VTL_vk_Publishers* pub,
                      VTL_vk_EmitFn emit, void* emit_ud);

/* Главная точка: разобрать текст сообщения от peer и выполнить команду. */
void VTL_vk_hub_Handle(VTL_vk_Hub* hub, long long peer_id, const char* text);

/* --- чистые помощники разбора (покрыты тестами) --- */
int  VTL_vk_ParseTarget(const char* word, VTL_content_platform_flags* out_bit);
int  VTL_vk_ParseMarkup(const char* word, VTL_publication_marked_text_MarkupType* out);
const char* VTL_vk_MarkupLabel(VTL_publication_marked_text_MarkupType markup);
void VTL_vk_TargetsLabel(VTL_content_platform_flags flags, char* out, size_t cap);
VTL_vk_Dialog* VTL_vk_hub_Dialog(VTL_vk_Hub* hub, long long peer_id);

#ifdef __cplusplus
}
#endif

#endif
