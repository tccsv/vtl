#include <VTL/user/VTL_user_data.h>

void VTL_user_Init(VTL_User* user) {
    memset(user, 0, sizeof(VTL_User));
    user->id = 0;
    user->has_last_name = false;
}

void VTL_user_Zeroize(VTL_User* user) {
    // Безопасное обнуление чувствительных данных
    memset(user->email, 0, sizeof(user->email));
    memset(user->first_name, 0, sizeof(user->first_name));
    memset(user->last_name, 0, sizeof(user->last_name));
    memset(user->nickname, 0, sizeof(user->nickname));
    user->id = 0;
    user->has_last_name = false;
}

void VTL_user_SetFirstName(VTL_User* user, const char* first_name) {
    snprintf(user->first_name, sizeof(user->first_name), "%s", first_name);
}

void VTL_user_SetLastName(VTL_User* user, const char* last_name) {
    if (last_name != NULL && strlen(last_name) > 0) {
        snprintf(user->last_name, sizeof(user->last_name), "%s", last_name);
        user->has_last_name = true;
    } else {
        user->has_last_name = false;
    }
}

void VTL_user_SetEmail(VTL_User* user, const char* email) {
    // Шифруем при сохранении
    VTL_StandartString plain_email;
    snprintf(plain_email, sizeof(plain_email), "%s", email);
    VTL_Encrypt(plain_email, user->email);

    // Затираем открытый текст
    memset(plain_email, 0, sizeof(plain_email));
}

void VTL_user_SetNickname(VTL_User* user, const char* nickname) {
    snprintf(user->nickname, sizeof(user->nickname), "%s", nickname);
}

void VTL_user_GetEmail(const VTL_User* user, VTL_StandartString decrypted_email) {
    VTL_Decrypt(user->email, decrypted_email);
}

void VTL_user_Print(const VTL_User* user) {
    VTL_StandartString email;
    VTL_user_GetEmail(user, email);

    printf("User #%d\n", user->id);
    printf("  Nickname:   %s\n", user->nickname);
    printf("  First name: %s\n", user->first_name);
    if (user->has_last_name) {
        printf("  Last name:  %s\n", user->last_name);
    }
    printf("  Email:      %s\n", email);

    // Затираем расшифрованную почту
    memset(email, 0, sizeof(email));
}