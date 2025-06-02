#ifndef _VTL_VIDEO_CONVERTER_H
#define _VTL_VIDEO_CONVERTER_H

#include "VTL/VTL_app_result.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Конвертирует видеофайл в другой формат
 * 
 * @param input_path Путь к исходному файлу
 * @param output_path Путь для сохранения результата
 * @param output_format Формат выходного файла (например "mp4", "avi")
 * @return Код результата операции (VTL_AppResult)
 */
VTL_AppResult VTL_convert_video_format(const char* input_path, 
                                     const char* output_path);

#ifdef __cplusplus
}
#endif

#endif