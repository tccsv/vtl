#ifndef _VTL_SUB_STYLE_H
#define _VTL_SUB_STYLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <VTL/VTL_app_result.h>

// Параметры оформления субтитров
typedef struct VTL_sub_StyleParams {
    const char* fontname;      // Семейство шрифтов
    int fontsize;              // Размер шрифта
    uint32_t primary_color;    // Цвет шрифта (ARGB)
    uint32_t back_color;       // Цвет фона (ARGB)
    uint32_t outline_color;    // Цвет контура (ARGB)
    int outline;               // Толщина контура
    int border_style;          // Стиль контура
    int bold;                  // Жирный
    int italic;                // Курсив
    int underline;             // Подчёркнутый
    int alignment;             // Выравнивание
    int margin_l, margin_r, margin_v; // Отступы
} VTL_sub_StyleParams;

// Загрузка параметров оформления из JSON-файла
VTL_AppResult VTL_sub_StyleLoadFromJson(const char* json_file, VTL_sub_StyleParams* style_params);

#ifdef __cplusplus
}
#endif

#endif // _VTL_SUB_STYLE_H 