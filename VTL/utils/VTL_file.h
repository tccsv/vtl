#ifndef _VTL_FILE_H
#define _VTL_FILE_H

#ifdef __cplusplus
extern "C"
{
#endif


#include <VTL/utils/VTL_string.h>
#include <VTL/VTL_app_result.h>
#include <VTL/utils/VTL_buffer_data.h>
#include <stddef.h>
#include <stdbool.h>

#define __STDC_WANT_LIB_EXT1__
#include <stdio.h>

typedef VTL_StandartString VTL_Filename;
typedef FILE VTL_File;
typedef int VTL_file_op_Result;

#define VTL_FILE_EOF EOF
#define VTL_FILE_OP_OK 0
#define VTL_FILENAME_MAX_LENGTH VTL_STANDART_STRING_MAX_LENGTH

VTL_AppResult VTL_file_OpenForReading(VTL_File** pp_file, VTL_Filename file_name);
VTL_AppResult VTL_file_OpenForWriting(VTL_File** pp_file, VTL_Filename file_name);
VTL_AppResult VTL_file_ReadRawData(VTL_BufferData** buffer_data, const VTL_Filename file_name);
VTL_AppResult VTL_file_WriteRawData(VTL_BufferData** buffer_data, const VTL_Filename file_name);
VTL_AppResult VTL_file_Copy(const VTL_Filename out_file_name, const VTL_Filename src_file_name);
bool VTL_file_CheckEquality(const VTL_Filename first_file_name, const VTL_Filename second_file_name);


#ifdef __cplusplus
}
#endif


#endif