#ifndef _VTL_USER_DATA_H
#define _VTL_USER_DATA_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/utils/VTL_encryptor.h>
#include <VTL/utils/VTL_time.h>
#include <stdio.h>
#include <stdbool.h>

typedef VTL_EncryptedString VTL_EncryptedUserData;

typedef struct _VTL_Error {
    VTL_StandartString nickname;
    VTL_StandartString error_message;
    VTL_Time error_time; 
} VTL_Error;

// Инициализация пустого пользователя
void VTL_error_Init(VTL_Error* error);

// Заполнение полей
void VTL_user_SetErrorTime(VTL_Error* error, VTL_Time errorTime);
void VTL_user_SetErrorMessage(VTL_Error* error, const char* message);
void VTL_user_SetNickname(VTL_Error* error, const char* nickname);


#ifdef __cplusplus
}
#endif


#endif