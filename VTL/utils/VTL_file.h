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

VTL_AppResult VTL_publication_FileOpenForReading(VTL_File** pp_file, VTL_Filename file_name);
VTL_AppResult VTL_publication_FileOpenForWriting(VTL_File** pp_file, VTL_Filename file_name);
VTL_AppResult VTL_publication_FileReadRawData(VTL_BufferData** buffer_data, const VTL_Filename file_name);
VTL_AppResult VTL_publication_FileWriteRawData(VTL_BufferData** buffer_data, const VTL_Filename file_name);
VTL_AppResult VTL_publication_FileCopy(const VTL_Filename out_file_name, const VTL_Filename src_file_name);
bool VTL_publication_FileCheckEquality(const VTL_Filename first_file_name, const VTL_Filename second_file_name);

#ifdef __cplusplus
}
#endif

#endif
