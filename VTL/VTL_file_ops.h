#ifndef _VTL_FILE_OPS_H
#define _VTL_FILE_OPS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>

/**
хедер для файловых операций
понимаю это как интерфейс для работы с файлами
*/
// структура с результатмси файловх операций
typedef enum _VTL_publication_FileResult
{
    VTL_publication_file_res_kOk = 0,
    VTL_publication_file_res_kErrorOpen,
    VTL_publication_file_res_kErrorRead,
    VTL_publication_file_res_kErrorWrite,
    VTL_publication_file_res_kErrorClose,
    VTL_publication_file_res_kErrorMemory
} VTL_publication_FileResult;

// операции с файлами
VTL_publication_FileResult VTL_publication_FileReadS(const char* filename, char** content, size_t* size);
VTL_publication_FileResult VTL_publication_FileWriteS(const char* filename, const char* content, size_t size);
VTL_publication_FileResult VTL_publication_FileAppendS(const char* filename, const char* content, size_t size);

// обработка ошибок
const char* VTL_publication_FileGetErrorMessage(VTL_publication_FileResult result);

#ifdef __cplusplus
}
#endif

#endif 
