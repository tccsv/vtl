#include "VTL/VTL.h"
#include "VTL/VTL_converter.h"
#include "VTL/VTL_file_ops.h"
#include "VTL/utils/log/VTL_console.h"
#include <stdio.h>

int main(void)
{
    VTL_publication_app_result app_result = VTL_publication_res_kOk;
    VTL_console_log(VTL_LOG_LEVEL_INFO, "Начало конвертации разметки...");

    // Пример 1: Конвертация стандартного MD в Telegram MD
    VTL_publication_ConversionConfig config1 = {
        .input_file = "src_text_file.md",
        .output_file = "out_tgmd.md",
        .input_type = VTL_publication_markup_type_kStandardMD,
        .output_type = VTL_publication_markup_type_kTelegramMD
    };
    
    VTL_console_log(VTL_LOG_LEVEL_INFO, "Конвертация стандартного MD в Telegram MD...");
    app_result = VTL_publication_convert_markup_s(&config1);
    if (app_result != VTL_publication_res_kOk) {
        VTL_console_log_file_error("Конвертация", config1.input_file, app_result);
        return app_result;
    }
    VTL_console_log(VTL_LOG_LEVEL_INFO, "Успешно конвертировано в Telegram MD");

    // Пример 2: Конвертация стандартного MD в обычный текст
    VTL_publication_ConversionConfig config2 = {
        .input_file = "src_text_file.md",
        .output_file = "out_plain.txt",
        .input_type = VTL_publication_markup_type_kStandardMD,
        .output_type = VTL_publication_markup_type_kPlainText
    };
    
    VTL_console_log(VTL_LOG_LEVEL_INFO, "Конвертация стандартного MD в обычный текст...");
    app_result = VTL_publication_convert_markup_s(&config2);
    if (app_result != VTL_publication_res_kOk) {
        VTL_console_log_file_error("Конвертация", config2.input_file, app_result);
        return app_result;
    }
    VTL_console_log(VTL_LOG_LEVEL_INFO, "Успешно конвертировано в обычный текст");

    // Пример 3: Конвертация Telegram MD обратно в стандартный MD
    VTL_publication_ConversionConfig config3 = {
        .input_file = "out_tgmd.md",
        .output_file = "out_stdmd.md",
        .input_type = VTL_publication_markup_type_kTelegramMD,
        .output_type = VTL_publication_markup_type_kStandardMD
    };
    
    VTL_console_log(VTL_LOG_LEVEL_INFO, "Конвертация Telegram MD обратно в стандартный MD...");
    app_result = VTL_publication_convert_markup_s(&config3);
    if (app_result != VTL_publication_res_kOk) {
        VTL_console_log_file_error("Конвертация", config3.input_file, app_result);
        return app_result;
    }
    VTL_console_log(VTL_LOG_LEVEL_INFO, "Успешно конвертировано обратно в стандартный MD");

    VTL_console_log(VTL_LOG_LEVEL_INFO, "Все конвертации успешно завершены");
    return app_result;
}