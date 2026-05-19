#include <VTL/history_administration/VTL_history_administration_data.h>
#include <stdio.h>
#include <string.h>

static const char* platform_to_string(VTL_Platform platform) {
    switch (platform) {
        case VTL_PLATFORM_TELEGRAM: return "Telegram";
        case VTL_PLATFORM_VK:       return "VK";
        default:                    return "Unknown";
    }
}

static const char* status_to_string(VTL_PublicationStatus status) {
    switch (status) {
        case VTL_PUBLICATION_DRAFT:     return "Draft";
        case VTL_PUBLICATION_SCHEDULED: return "Scheduled";
        case VTL_PUBLICATION_PUBLISHED: return "Published";
        case VTL_PUBLICATION_FAILED:    return "Failed";
        default:                        return "Unknown";
    }
}

static const char* type_to_string(VTL_PublicationType type) {
    switch (type) {
        case VTL_PUBLICATION_TEXT_ONLY:       return "Text only";
        case VTL_PUBLICATION_MEDIA_ONLY:      return "Media only";
        case VTL_PUBLICATION_MEDIA_WITH_TEXT:  return "Media + Text";
        default:                              return "Unknown";
    }
}

static void history_base_init(VTL_UserHistory* history, const VTL_User* user,
                               VTL_Platform platform, VTL_content_platform_flags flags) {
    memset(history, 0, sizeof(VTL_UserHistory));
    history->id = 0;
    history->user = *user;
    history->platform = platform;
    history->flags = flags;
    history->status = VTL_PUBLICATION_PUBLISHED;
    history->has_text_file = false;
    history->has_media_file = false;
    // publication_start_time = сейчас
    time_t now = time(NULL);
    history->publication_start_time = (VTL_Time)time(NULL);
}

void VTL_user_history_text_publication_Init(
        VTL_UserHistory* history,
        const VTL_User* user,
        const VTL_Filename text_file_name,
        VTL_Platform platform,
        VTL_content_platform_flags flags) {

    history_base_init(history, user, platform, flags);
    snprintf(history->text_file_name, sizeof(history->text_file_name), "%s", text_file_name);
    history->has_text_file = true;
    history->publication_type = VTL_PUBLICATION_TEXT_ONLY;
}

void VTL_user_history_media_publication_Init(
        VTL_UserHistory* history,
        const VTL_User* user,
        const VTL_Filename media_file_name,
        VTL_Platform platform,
        VTL_content_platform_flags flags) {

    history_base_init(history, user, platform, flags);
    snprintf(history->media_file_name, sizeof(history->media_file_name), "%s", media_file_name);
    history->has_media_file = true;
    history->publication_type = VTL_PUBLICATION_MEDIA_ONLY;
}

void VTL_user_history_media_w_text_publication_Init(
        VTL_UserHistory* history,
        const VTL_User* user,
        const VTL_Filename text_file_name,
        const VTL_Filename media_file_name,
        VTL_Platform platform,
        VTL_content_platform_flags flags) {

    history_base_init(history, user, platform, flags);
    snprintf(history->text_file_name, sizeof(history->text_file_name), "%s", text_file_name);
    snprintf(history->media_file_name, sizeof(history->media_file_name), "%s", media_file_name);
    history->has_text_file = true;
    history->has_media_file = true;
    history->publication_type = VTL_PUBLICATION_MEDIA_WITH_TEXT;
}

void VTL_user_history_scheduled_Init(
        VTL_UserHistory* history,
        const VTL_User* user,
        const VTL_Filename text_file_name,
        const VTL_Filename media_file_name,
        VTL_Platform platform,
        VTL_content_platform_flags flags,
        VTL_Time scheduled_time) {

    history_base_init(history, user, platform, flags);

    if (text_file_name != NULL && strlen(text_file_name) > 0) {
        snprintf(history->text_file_name, sizeof(history->text_file_name), "%s", text_file_name);
        history->has_text_file = true;
    }
    if (media_file_name != NULL && strlen(media_file_name) > 0) {
        snprintf(history->media_file_name, sizeof(history->media_file_name), "%s", media_file_name);
        history->has_media_file = true;
    }

    if (history->has_text_file && history->has_media_file) {
        history->publication_type = VTL_PUBLICATION_MEDIA_WITH_TEXT;
    } else if (history->has_media_file) {
        history->publication_type = VTL_PUBLICATION_MEDIA_ONLY;
    } else {
        history->publication_type = VTL_PUBLICATION_TEXT_ONLY;
    }

    history->publication_start_time = scheduled_time;
    history->status = VTL_PUBLICATION_SCHEDULED;
}

void VTL_user_history_SetStatus(VTL_UserHistory* history, VTL_PublicationStatus status) {
    history->status = status;
}

void VTL_user_history_Zeroize(VTL_UserHistory* history) {
    VTL_user_Zeroize(&history->user);
    memset(history, 0, sizeof(VTL_UserHistory));
}

void VTL_user_history_Print(const VTL_UserHistory* history) {
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &history->publication_start_time);

    printf("Publication #%d\n", history->id);
    printf("  User:      @%s\n", history->user.nickname);
    printf("  Platform:  %s\n", platform_to_string(history->platform));
    printf("  Type:      %s\n", type_to_string(history->publication_type));
    printf("  Status:    %s\n", status_to_string(history->status));
    printf("  Time:      %s\n", time_buf);

    if (history->has_text_file) {
        printf("  Text file: %s\n", history->text_file_name);
    }
    if (history->has_media_file) {
        printf("  Media file: %s\n", history->media_file_name);
    }
}