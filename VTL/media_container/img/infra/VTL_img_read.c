#include "VTL_img_read.h"
#include <stdlib.h>
#include <string.h>

VTL_AppResult VTL_img_read_from_file(const char* filename, VTL_img_data_t* img_data)
{
    if (!filename || !img_data) {
        return VTL_IMG_ERROR_INVALID_PARAM;
    }

    memset(img_data, 0, sizeof(VTL_img_data_t));

    if (avformat_open_input(&img_data->format_ctx, filename, NULL, NULL) < 0) {
        return VTL_IMG_ERROR_FILE_NOT_FOUND;
    }

    if (avformat_find_stream_info(img_data->format_ctx, NULL) < 0) {
        avformat_close_input(&img_data->format_ctx);
        return VTL_IMG_ERROR_FILE_ACCESS;
    }

    int video_stream_index = av_find_best_stream(
        img_data->format_ctx,
        AVMEDIA_TYPE_VIDEO,
        -1,
        -1,
        NULL,
        0
    );

    if (video_stream_index < 0) {
        avformat_close_input(&img_data->format_ctx);
        return VTL_IMG_ERROR_FORMAT_NOT_SUPPORTED;
    }

    const AVCodec* decoder = avcodec_find_decoder(
        img_data->format_ctx->streams[video_stream_index]->codecpar->codec_id
    );

    if (!decoder) {
        avformat_close_input(&img_data->format_ctx);
        return VTL_IMG_ERROR_FORMAT_NOT_SUPPORTED;
    }

    img_data->codec_ctx = avcodec_alloc_context3(decoder);
    if (!img_data->codec_ctx) {
        avformat_close_input(&img_data->format_ctx);
        return VTL_IMG_ERROR_MEMORY;
    }

    if (avcodec_parameters_to_context(img_data->codec_ctx, 
        img_data->format_ctx->streams[video_stream_index]->codecpar) < 0) {
        avcodec_free_context(&img_data->codec_ctx);
        avformat_close_input(&img_data->format_ctx);
        return VTL_IMG_ERROR_DECODE;
    }

    if (avcodec_open2(img_data->codec_ctx, decoder, NULL) < 0) {
        avcodec_free_context(&img_data->codec_ctx);
        avformat_close_input(&img_data->format_ctx);
        return VTL_IMG_ERROR_DECODE;
    }

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    int ret = VTL_IMG_ERROR_DECODE;

    while (av_read_frame(img_data->format_ctx, packet) >= 0) {
        if (packet->stream_index == video_stream_index) {
            if (avcodec_send_packet(img_data->codec_ctx, packet) < 0) {
                break;
            }
            if (avcodec_receive_frame(img_data->codec_ctx, frame) == 0) {
                img_data->current_frame = av_frame_alloc();
                if (!img_data->current_frame) {
                    break;
                }
                if (av_frame_ref(img_data->current_frame, frame) < 0) {
                    av_frame_free(&img_data->current_frame);
                    break;
                }
                ret = VTL_IMG_SUCCESS;
                break;
            }
        }
        av_packet_unref(packet);
    }

    av_frame_free(&frame);
    av_packet_free(&packet);

    if (ret != VTL_IMG_SUCCESS) {
        avcodec_free_context(&img_data->codec_ctx);
        avformat_close_input(&img_data->format_ctx);
    }

    return ret;
}