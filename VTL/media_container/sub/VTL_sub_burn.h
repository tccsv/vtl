#ifndef VTL_SUB_BURN_H
#define VTL_SUB_BURN_H

#include <VTL/media_container/sub/VTL_sub_data.h> // Для VTL_sub_Format
#include <VTL/media_container/sub/VTL_sub_style.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Встраивает субтитры в видеофайл с использованием библиотек FFmpeg.
 *
 * @param input_video Путь к входному видеофайлу.
 * @param input_subs Путь к файлу субтитров.
 * @param subs_format Формат субтитров (в текущей реализации libavfilter subtitles используется автоопределение).
 * @param output_video Путь к выходному видеофайлу со встроенными субтитрами.
 * @param style_params Параметры стиля субтитров.
 * @return 0 в случае успеха, -1 или другой код ошибки FFmpeg в случае неудачи.
 */
VTL_AppResult VTL_sub_BurnToVideo(const char* input_video, const char* input_subs, VTL_sub_Format subs_format, const char* output_video, const VTL_sub_StyleParams* style_params);

#ifdef __cplusplus
}
#endif

#endif // VTL_SUB_BURN_H 