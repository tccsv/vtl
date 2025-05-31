#ifndef VTL_CONSOLE_H
#define VTL_CONSOLE_H

#include "VTL_app_result.h"
#include <stdarg.h>

// Уровни логирования
typedef enum {
    VTL_LOG_LEVEL_INFO,    // Информация
    VTL_LOG_LEVEL_WARNING, // Предупреждение
    VTL_LOG_LEVEL_ERROR,   // Ошибка
    VTL_LOG_LEVEL_DEBUG    // Отладочная информация
} VTL_LogLevel;

// Функции вывода в консоль
void VTL_console_log(VTL_LogLevel level, const char* format, ...);
void VTL_console_log_error(VTL_AppResult app_result);
void VTL_console_log_file_error(const char* operation, const char* filename, VTL_AppResult result);

// Функции прямого вывода
void VTL_console_output(const char* message);
void VTL_console_output_formatted(const char* format, ...);

#endif // VTL_CONSOLE_H