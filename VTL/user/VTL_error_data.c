#include <VTL/user/VTL_error_data.h>

void VTL_error_Init(VTL_Error* error) {
    memset(error, 0, sizeof(VTL_Error));
    memset(error->nickname, 0, sizeof(error->nickname));
    memset(error->error_message, 0, sizeof(error->error_message));
    time_t now = time(NULL);
    error->error_time = (VTL_Time)time(NULL);
}

void VTL_user_SetNickname(VTL_Error* error, const char* nickname) {
    snprintf(error->nickname, sizeof(error->nickname), "%s", nickname);
}

void VTL_user_SetErrorMessage(VTL_Error* error, const char* message) {
    snprintf(error->error_message, sizeof(error->error_message), "%s", message);
}

void VTL_user_SetErrorTime(VTL_Error* error, VTL_Time errorTime) {
    error->error_time = errorTime;
}