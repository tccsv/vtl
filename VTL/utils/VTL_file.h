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

#include <stdio.h>


#define VTL_publication_file_name_size VTL_publication_string_size

typedef VTL_publication_string VTL_Filename;
typedef FILE VTL_File;


VTL_publication_app_result VTL_publication_file_OpenForReading(VTL_File** pp_file, VTL_Filename file_name);
VTL_publication_app_result VTL_publication_file_OpenForWriting(VTL_File** pp_file, VTL_Filename file_name);
VTL_publication_app_result VTL_publication_file_ReadRawData(VTL_BufferData** buffer_data, const VTL_Filename file_name);
VTL_publication_app_result VTL_publication_file_WriteRawData(VTL_BufferData** buffer_data, const VTL_Filename file_name);
VTL_publication_app_result VTL_publication_file_Copy(const VTL_Filename out_file_name, const VTL_Filename src_file_name);
bool VTL_publication_file_CheckEquality(const VTL_Filename first_file_name, const VTL_Filename second_file_name);


#ifdef __cplusplus
}
#endif


#endif