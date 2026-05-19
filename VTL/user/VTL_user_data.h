#ifndef _VTL_USER_DATA_H
#define _VTL_USER_DATA_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/utils/VTL_encryptor.h>
#include <stdio.h>
#include <stdbool.h>

typedef VTL_EncryptedString VTL_EncryptedUserData;

typedef struct _VTL_User {
    int id;                              // ID из БД (0 = новый)
    VTL_StandartString first_name;       // Имя (обязательное)
    VTL_StandartString last_name;        // Фамилия (опциональное)
    bool has_last_name;                  // Есть ли фамилия
    VTL_EncryptedString email;           // Почта (зашифрована)
    VTL_StandartString nickname;         // Никнейм
} VTL_User;

// Инициализация пустого пользователя
void VTL_user_Init(VTL_User* user);

// Обнуление (очистка чувствительных данных)
void VTL_user_Zeroize(VTL_User* user);

// Заполнение полей
void VTL_user_SetFirstName(VTL_User* user, const char* first_name);
void VTL_user_SetLastName(VTL_User* user, const char* last_name);
void VTL_user_SetEmail(VTL_User* user, const char* email);
void VTL_user_SetNickname(VTL_User* user, const char* nickname);

// Получение расшифрованной почты
void VTL_user_GetEmail(const VTL_User* user, VTL_StandartString decrypted_email);

// Печать пользователя
void VTL_user_Print(const VTL_User* user);


#ifdef __cplusplus
}
#endif


#endif