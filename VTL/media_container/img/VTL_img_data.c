#include "VTL_img_data.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Инициализация структуры данных изображения
VTL_AppResult VTL_img_data_init(VTL_img_data_t* img_data)
{
    if (!img_data) {
        return VTL_IMG_ERROR_INVALID_PARAM;
    }

    memset(img_data, 0, sizeof(VTL_img_data_t));
    return VTL_IMG_SUCCESS;
}

// Освобождение ресурсов
void VTL_img_data_free(VTL_img_data_t* img_data)
{
    if (!img_data) {
        return;
    }

    if (img_data->current_frame) {
        av_frame_free(&img_data->current_frame);
    }

    if (img_data->codec_ctx) {
        avcodec_free_context(&img_data->codec_ctx);
    }

    if (img_data->format_ctx) {
        avformat_close_input(&img_data->format_ctx);
    }

    if (img_data->filter_graph) {
        avfilter_graph_free(&img_data->filter_graph);
    }

    if (img_data->sws_ctx) {
        sws_freeContext(img_data->sws_ctx);
    }

    memset(img_data, 0, sizeof(VTL_img_data_t));
}

// Инициализация фильтра
static int init_filter_graph(VTL_img_data_t* img_data, const char* filter_descr)
{
    if (!img_data || !img_data->current_frame) {
        return VTL_IMG_ERROR_INVALID_PARAM;
    }

    char args[512];
    AVFilterInOut *inputs = avfilter_inout_alloc();
    AVFilterInOut *outputs = avfilter_inout_alloc();

    img_data->filter_graph = avfilter_graph_alloc();

    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=1/25:pixel_aspect=1/1",
             img_data->current_frame->width, img_data->current_frame->height, img_data->current_frame->format);

    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");

    avfilter_graph_create_filter(&img_data->buffersrc_ctx, buffersrc, "in", args, NULL, img_data->filter_graph);
    avfilter_graph_create_filter(&img_data->buffersink_ctx, buffersink, "out", NULL, NULL, img_data->filter_graph);

    outputs->name = av_strdup("in");
    outputs->filter_ctx = img_data->buffersrc_ctx;
    outputs->pad_idx = 0;
    outputs->next = NULL;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = img_data->buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = NULL;

    avfilter_graph_parse_ptr(img_data->filter_graph, filter_descr, &inputs, &outputs, NULL);
    avfilter_graph_config(img_data->filter_graph, NULL);

    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    return VTL_IMG_SUCCESS;
}

// Применение фильтра к изображению
VTL_AppResult VTL_img_apply_filter(VTL_img_data_t* img_data, const VTL_ImageFilter* filter)
{
    if (!img_data || !filter || !img_data->current_frame) {
        return VTL_IMG_ERROR_INVALID_PARAM;
    }

    // Инициализируем граф фильтров
    int ret = init_filter_graph(img_data, filter->filter_desc);
    if (ret != VTL_IMG_SUCCESS) {
        return ret;
    }

    // Отправляем кадр в фильтр
    if (av_buffersrc_add_frame_flags(img_data->buffersrc_ctx, img_data->current_frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
        return VTL_IMG_ERROR_FILTER;
    }

    // Получаем отфильтрованный кадр
    AVFrame* filtered_frame = av_frame_alloc();
    if (!filtered_frame) {
        return VTL_IMG_ERROR_MEMORY;
    }

    ret = av_buffersink_get_frame(img_data->buffersink_ctx, filtered_frame);
    if (ret < 0) {
        av_frame_free(&filtered_frame);
        return VTL_IMG_ERROR_FILTER;
    }

    // Заменяем текущий кадр отфильтрованным
    av_frame_free(&img_data->current_frame);
    img_data->current_frame = filtered_frame;

    return VTL_IMG_SUCCESS;
}

// Конвертация формата изображения
VTL_AppResult VTL_img_convert_format(VTL_img_data_t* img_data, enum AVPixelFormat dst_format)
{
    if (!img_data || !img_data->current_frame) {
        return VTL_IMG_ERROR_INVALID_PARAM;
    }

    if (img_data->current_frame->format == dst_format) {
        return VTL_IMG_SUCCESS;
    }

    // Создаем контекст для конвертации
    img_data->sws_ctx = sws_getContext(
        img_data->current_frame->width,
        img_data->current_frame->height,
        img_data->current_frame->format,
        img_data->current_frame->width,
        img_data->current_frame->height,
        dst_format,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
    );

    if (!img_data->sws_ctx) {
        return VTL_IMG_ERROR_MEMORY;
    }

    // Создаем новый кадр для конвертированного изображения
    AVFrame* dst_frame = av_frame_alloc();
    if (!dst_frame) {
        sws_freeContext(img_data->sws_ctx);
        img_data->sws_ctx = NULL;
        return VTL_IMG_ERROR_MEMORY;
    }

    dst_frame->format = dst_format;
    dst_frame->width = img_data->current_frame->width;
    dst_frame->height = img_data->current_frame->height;

    if (av_frame_get_buffer(dst_frame, 0) < 0) {
        av_frame_free(&dst_frame);
        sws_freeContext(img_data->sws_ctx);
        img_data->sws_ctx = NULL;
        return VTL_IMG_ERROR_MEMORY;
    }

    // Конвертируем изображение
    if (sws_scale(
        img_data->sws_ctx,
        (const uint8_t* const*)img_data->current_frame->data,
        img_data->current_frame->linesize,
        0,
        img_data->current_frame->height,
        dst_frame->data,
        dst_frame->linesize
    ) < 0) {
        av_frame_free(&dst_frame);
        sws_freeContext(img_data->sws_ctx);
        img_data->sws_ctx = NULL;
        return VTL_IMG_ERROR_DECODE;
    }

    // Освобождаем старый кадр и устанавливаем новый
    av_frame_free(&img_data->current_frame);
    img_data->current_frame = dst_frame;

    return VTL_IMG_SUCCESS;
} 