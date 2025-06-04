#ifndef _VTL_ENCRYPTOR_H
#define _VTL_ENCRYPTOR_H

#ifdef __cplusplus
extern "C"
{
#endif


#include <VTL/utils/VTL_string.h>


#define VTL_ENCRYPTED_STRING_MAX_LENGTH VTL_STANDART_STRING_MAX_LENGTH

typedef VTL_StandartChar VTL_EncryptedChar;

#define VTL_ENCRYPTED_STRING_SIZE VTL_STANDART_STRING_SIZE

typedef VTL_StandartString VTL_EncryptedString;


void VTL_Encrypt(const VTL_StandartString src_string, VTL_EncryptedString encrypted_string);



#ifdef __cplusplus
}
#endif


#endif