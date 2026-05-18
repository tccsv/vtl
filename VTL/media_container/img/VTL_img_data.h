#ifndef _VTL_IMG_DATA_H
#define _VTL_IMG_DATA_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

typedef struct _VTL_ImageFilter {
    const char* name;
    const char* description;
    const char* filter_desc;  // FFmpeg filter description string
    int (*apply)(AVFrame* frame);
} VTL_ImageFilter;

typedef struct _VTL_ImageContext {
    AVFormatContext* format_ctx;
    AVCodecContext* codec_ctx;
    AVFilterContext* buffersrc_ctx;
    AVFilterContext* buffersink_ctx;
    AVFilterGraph* filter_graph;
    SwsContext* sws_ctx;
    AVFrame* current_frame;  // Текущий кадр изображения
} VTL_ImageContext;

#ifdef __cplusplus
}
#endif

#endif // _VTL_IMG_DATA_H