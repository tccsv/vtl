#pragma once
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>

// Загружает изображение из файла в AVFrame. Возвращает 0 при успехе, иначе <0.
int VTL_img_load(const char* filename, AVFrame** out_frame, int* width, int* height, enum AVPixelFormat* pix_fmt); 