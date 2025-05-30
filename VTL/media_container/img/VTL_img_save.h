#pragma once
#include <libavutil/frame.h>

// Сохраняет AVFrame как PNG/JPEG/BMP по указанному пути. Возвращает 0 при успехе, иначе <0.
int VTL_img_save(const char* filename, AVFrame* frame); 