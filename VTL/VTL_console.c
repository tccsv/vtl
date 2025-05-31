#include "VTL_console.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

// ANSI коды цветов
#define VTL_COLOR_RED     "\x1b[31m"
#define VTL_COLOR_GREEN   "\x1b[32m"
#define VTL_COLOR_YELLOW  "\x1b[33m"
#define VTL_COLOR_BLUE    "\x1b[34m"
#define VTL_COLOR_RESET   "\x1b[0m"

// Сообщения об ошибках
static const char* VTL_ERROR_MESSAGES[] = {
    [VTL_res_kOk] = "Операция успешно завершена",
    [VTL_res_kError] = "Произошла общая ошибка",
    [VTL_res_video_fs_r_kMissingFileErr] = "Входной файл отсутствует",
    [VTL_res_video_fs_r_kFileIsBusyErr] = "Входной файл занят",
    [VTL_FILE_RES_ERROR_OPEN] = "Не удалось открыть файл",
    [VTL_FILE_RES_ERROR_READ] = "Не удалось прочитать файл",
    [VTL_FILE_RES_ERROR_WRITE] = "Не удалось записать в файл",
    [VTL_FILE_RES_ERROR_CLOSE] = "Не удалось закрыть файл",
    [VTL_FILE_RES_ERROR_MEMORY] = "Ошибка выделения памяти"
};

// Получение текущей временной метки
static void VTL_get_timestamp(char* buffer, size_t size) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

// Получение строки уровня логирования и цвета
static const char* VTL_get_log_level_info(VTL_LogLevel level, const char** color) {
    switch (level) {
        case VTL_LOG_LEVEL_INFO:
            *color = VTL_COLOR_GREEN;
            return "ИНФО";
        case VTL_LOG_LEVEL_WARNING:
            *color = VTL_COLOR_YELLOW;
            return "ПРЕДУПРЕЖДЕНИЕ";
        case VTL_LOG_LEVEL_ERROR:
            *color = VTL_COLOR_RED;
            return "ОШИБКА";
        case VTL_LOG_LEVEL_DEBUG:
            *color = VTL_COLOR_BLUE;
            return "ОТЛАДКА";
        default:
            *color = VTL_COLOR_RESET;
            return "НЕИЗВЕСТНО";
    }
}

void VTL_console_log(VTL_LogLevel level, const char* format, ...) {
    char timestamp[20];
    const char* color;
    const char* level_str = VTL_get_log_level_info(level, &color);
    
    VTL_get_timestamp(timestamp, sizeof(timestamp));
    
    // Вывод временной метки и уровня
    printf("%s[%s] %s%s: ", color, timestamp, level_str, VTL_COLOR_RESET);
    
    // Вывод сообщения
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
}

void VTL_console_log_error(VTL_AppResult app_result) {
    if (app_result == VTL_res_kOk) {
        return;
    }

    const char* error_msg = "Неизвестная ошибка";
    if (app_result >= 0 && app_result < sizeof(VTL_ERROR_MESSAGES) / sizeof(VTL_ERROR_MESSAGES[0])) {
        error_msg = VTL_ERROR_MESSAGES[app_result];
    }

    VTL_console_log(VTL_LOG_LEVEL_ERROR, "%s", error_msg);
}

void VTL_console_log_file_error(const char* operation, const char* filename, VTL_AppResult result) {
    VTL_console_log(VTL_LOG_LEVEL_ERROR, "%s не удалось для файла '%s': %s", 
        operation, filename, VTL_ERROR_MESSAGES[result]);
}

void VTL_console_output(const char* message) {
    printf("%s\n", message);
}

void VTL_console_output_formatted(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}