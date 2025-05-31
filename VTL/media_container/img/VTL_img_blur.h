#pragma once
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>

// Применяет размытие к AVFrame через фильтр boxblur. Возвращает 0 при успехе, иначе <0.
int VTL_img_Blur(AVFrame* in_frame, AVFrame** out_frame, int width, int height, enum AVPixelFormat pix_fmt, const char* blur_params); 