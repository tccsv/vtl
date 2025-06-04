#include <VTL/user/VTL_user_data.h>

void VTL_user_Init(VTL_User* p_user)
{
    const char encrypted_user_data[] = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VyX25hbWUiOiJleGFtcGxlX3VzZXJfbmFtZSJ9.h9RZCSAuPhxdFa65XtlfupxExWiOlT0sz4f7qU8XDcQ";
    snprintf(p_user->nickname, sizeof(p_user->nickname), "%s", encrypted_user_data);
    return;
}

void VTL_user_Zeroize(VTL_User* p_user)
{
    return;
}