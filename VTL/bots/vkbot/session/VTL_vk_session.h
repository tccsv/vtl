#ifndef VTL_VK_SESSION_H
#define VTL_VK_SESSION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <VTL/bots/vkbot/VTL_vk_bot.h>
#include <VTL/bots/vkbot/VTL_vk_data.h>

#include <stddef.h>

/* Состояние диалогов и помощники разбора. */

#define VTL_VK_MAX_DIALOGS 64
#define VTL_VK_PATH_MAX    256
#define VTL_VK_TOKEN_MAX   128

/* Выбор для одного диалога. */
typedef struct VTL_vk_Dialog {
    long long                               peer_id;
    VTL_content_platform_flags              targets;
    VTL_publication_marked_text_MarkupType  markup;
    char                                    source[VTL_VK_PATH_MAX];
    char                                    token[VTL_VK_TOKEN_MAX];
    int                                     used;
} VTL_vk_Dialog;

/* Канал ответа. */
typedef void (*VTL_vk_EmitFn)(void* ud, long long peer_id, const char* text);

typedef struct VTL_vk_Hub {
    VTL_vk_Dialog            dialogs[VTL_VK_MAX_DIALOGS];
    const VTL_vk_Publishers* pub;
    VTL_vk_EmitFn            emit;
    void*                    emit_ud;
} VTL_vk_Hub;

void VTL_vk_hub_Reset(VTL_vk_Hub* hub, const VTL_vk_Publishers* pub,
                      VTL_vk_EmitFn emit, void* emit_ud);
VTL_vk_Dialog* VTL_vk_hub_Dialog(VTL_vk_Hub* hub, long long peer_id);
void VTL_vk_session_ResetDefaults(VTL_vk_Dialog* d, long long peer_id);

int  VTL_vk_ParseTarget(const char* word, VTL_content_platform_flags* out_bit);
int  VTL_vk_ParseMarkup(const char* word, VTL_publication_marked_text_MarkupType* out);
const char* VTL_vk_MarkupLabel(VTL_publication_marked_text_MarkupType markup);
void VTL_vk_TargetsLabel(VTL_content_platform_flags flags, char* out, size_t cap);
const char* VTL_vk_TargetsHelp(void);
const char* VTL_vk_MarkupsHelp(void);

void VTL_vk_session_SetTargets(VTL_vk_Dialog* d, VTL_content_platform_flags flags);
void VTL_vk_session_SetMarkup(VTL_vk_Dialog* d, VTL_publication_marked_text_MarkupType markup);
void VTL_vk_session_SetSource(VTL_vk_Dialog* d, const char* path);
void VTL_vk_session_SetToken(VTL_vk_Dialog* d, const char* token);
void VTL_vk_session_ClearToken(VTL_vk_Dialog* d);
int  VTL_vk_session_HasToken(const VTL_vk_Dialog* d);

#ifdef __cplusplus
}
#endif

#endif
