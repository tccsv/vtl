#ifndef _VTL_AUDIO_FILE_H
#define _VTL_AUDIO_FILE_H

#ifdef __cplusplus
extern "C"
{
#endif


#include <VTL/utils/VTL_file.h>


#define VTL_AUDIO_FILE_EOF VTL_FILE_EOF
#define VTL_AUDIO_FILE_OP_OK VTL_FILE_OP_OK

typedef VTL_file_op_Result VTL_audio_file_op_Result; 
typedef VTL_File VTL_audio_File;


#ifdef __cplusplus
}
#endif

#endif