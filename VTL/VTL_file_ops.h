#ifndef _VTL_FILE_OPS_H
#define _VTL_FILE_OPS_H

#include "VTL_app_result.h"
#include <stddef.h>

/**
хедер для файловых операций
понимаю это как интерфейс для работы с файлами
*/
// структура с результатмси файловх операций
typedef enum {
    VTL_publication_file_res_kOk = 0,
    VTL_publication_file_res_kErrorOpen,
    VTL_publication_file_res_kErrorRead,
    VTL_publication_file_res_kErrorWrite,
    VTL_publication_file_res_kErrorClose,
    VTL_publication_file_res_kErrorMemory
} VTL_publication_file_result;

// операции с файлами
VTL_publication_file_result VTL_publication_file_read_s(const char* filename, char** content, size_t* size);
VTL_publication_file_result VTL_publication_file_write_s(const char* filename, const char* content, size_t size);
VTL_publication_file_result VTL_publication_file_append_s(const char* filename, const char* content, size_t size);

// обработка ошибок
const char* VTL_publication_file_get_error_message(VTL_publication_file_result result);

#endif 