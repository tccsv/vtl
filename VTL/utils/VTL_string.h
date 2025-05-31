#ifndef _VTL_STRING_H
#define _VTL_STRING_H

#ifdef __cplusplus
extern "C"
{
#endif


#define VTL_STANDART_STRING_MAX_LENGTH 1024

typedef char VTL_StandartChar;

#define VTL_STANDART_STRING_SIZE (VTL_STANDART_STRING_MAX_LENGTH*sizeof(VTL_StandartChar))

typedef VTL_StandartChar VTL_StandartString[VTL_STANDART_STRING_MAX_LENGTH];


#ifdef __cplusplus
}
#endif


#endif