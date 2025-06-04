#ifndef _VTL_USER_DATA_H
#define _VTL_USER_DATA_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/utils/VTL_encryptor.h>
#include <stdio.h>

typedef VTL_EncryptedString VTL_EncryptedUserData;

typedef struct _VTL_User
{
    VTL_EncryptedUserData nickname;
} VTL_User;

void VTL_user_Init(VTL_User* p_user);
void VTL_user_Zeroize(VTL_User* p_user);


#ifdef __cplusplus
}
#endif


#endif