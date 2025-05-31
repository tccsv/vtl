#include "VTL_img_file.h"
#include <stdlib.h>
#include <string.h>

VTL_AppResult VTL_img_file_get_info(const char* filename, VTL_img_data_t* img_data)
{
    if (!filename || !img_data) {
        return VTL_IMG_ERROR_INVALID_PARAM;
    }

    AVFormatContext* format_ctx = NULL;
    int ret = VTL_IMG_ERROR_FILE_NOT_FOUND;

    if (avformat_open_input(&format_ctx, filename, NULL, NULL) < 0) {
        return VTL_IMG_ERROR_FILE_NOT_FOUND;
    }

    if (avformat_find_stream_info(format_ctx, NULL) < 0) {
        avformat_close_input(&format_ctx);
        return VTL_IMG_ERROR_FILE_ACCESS;
    }

    int video_stream_index = av_find_best_stream(
        format_ctx,
        AVMEDIA_TYPE_VIDEO,
        -1,
        -1,
        NULL,
        0
    );

    if (video_stream_index < 0) {
        avformat_close_input(&format_ctx);
        return VTL_IMG_ERROR_FORMAT_NOT_SUPPORTED;
    }

    const AVCodecParameters* codecpar = format_ctx->streams[video_stream_index]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);

    if (!codec) {
        avformat_close_input(&format_ctx);
        return VTL_IMG_ERROR_FORMAT_NOT_SUPPORTED;
    }

    img_data->codec_ctx = avcodec_alloc_context3(codec);
    if (!img_data->codec_ctx) {
        avformat_close_input(&format_ctx);
        return VTL_IMG_ERROR_MEMORY;
    }

    if (avcodec_parameters_to_context(img_data->codec_ctx, codecpar) < 0) {
        avcodec_free_context(&img_data->codec_ctx);
        avformat_close_input(&format_ctx);
        return VTL_IMG_ERROR_DECODE;
    }

    img_data->format_ctx = format_ctx;

    return VTL_IMG_SUCCESS;
}

VTL_AppResult VTL_img_file_validate(const char* filename)
{
    if (!filename) {
        return VTL_IMG_ERROR_INVALID_PARAM;
    }

    AVFormatContext* format_ctx = NULL;
    int ret = VTL_IMG_ERROR_FILE_NOT_FOUND;

    if (avformat_open_input(&format_ctx, filename, NULL, NULL) < 0) {
        return VTL_IMG_ERROR_FILE_NOT_FOUND;
    }

    if (avformat_find_stream_info(format_ctx, NULL) < 0) {
        avformat_close_input(&format_ctx);
        return VTL_IMG_ERROR_FILE_ACCESS;
    }

    int video_stream_index = av_find_best_stream(
        format_ctx,
        AVMEDIA_TYPE_VIDEO,
        -1,
        -1,
        NULL,
        0
    );

    if (video_stream_index < 0) {
        avformat_close_input(&format_ctx);
        return VTL_IMG_ERROR_FORMAT_NOT_SUPPORTED;
    }

    // Проверяем кодек
    const AVCodecParameters* codecpar = format_ctx->streams[video_stream_index]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);

    if (!codec) {
        avformat_close_input(&format_ctx);
        return VTL_IMG_ERROR_FORMAT_NOT_SUPPORTED;
    }

    avformat_close_input(&format_ctx);
    return VTL_IMG_SUCCESS;
} 