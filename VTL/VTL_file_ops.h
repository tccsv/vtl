#ifndef VTL_FILE_OPS_H
#define VTL_FILE_OPS_H

#include "VTL_app_result.h"
#include <stddef.h>

/**
хедер для файловых операций
понимаю это как интерфейс для работы с файлами
*/
// структура с результатмси файловых операций
typedef enum {
    VTL_FILE_RES_OK = 0,
    VTL_FILE_RES_ERROR_OPEN,
    VTL_FILE_RES_ERROR_READ,
    VTL_FILE_RES_ERROR_WRITE,
    VTL_FILE_RES_ERROR_CLOSE,
    VTL_FILE_RES_ERROR_MEMORY
} VTL_FileResult;

// операции с файлами
VTL_FileResult VTL_file_read_s(const char* filename, char** content, size_t* size);
VTL_FileResult VTL_file_write_s(const char* filename, const char* content, size_t size);
VTL_FileResult VTL_file_append_s(const char* filename, const char* content, size_t size);

// обработка ошибок
const char* VTL_file_get_error_message(VTL_FileResult result);

#endif 