#ifndef _VTL_PUBLICATION_TEXT_WRITE_H
#define _VTL_PUBLICATION_TEXT_WRITE_H

#ifdef __cplusplus
extern "C"
{
#endif


#include <VTL/publication/text/VTL_publication_text_data.h>
#include <VTL/utils/VTL_file.h>
#include <VTL/VTL_app_result.h>


VTL_AppResult VTL_pusblication_text_Write(VTL_publication_Text* p_text, const VTL_Filename file_name);


#ifdef __cplusplus
}
#endif


#endif