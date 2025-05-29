#include "VTL_img_write.h"
#include <stdlib.h>
#include <string.h>

static void cleanup_resources(AVFormatContext* out_fmt_ctx, 
                            AVCodecContext* enc_ctx,
                            AVFrame* rgb_frame,
                            AVPacket* packet)
{
    if (out_fmt_ctx) {
        avio_closep(&out_fmt_ctx->pb);
        avformat_free_context(out_fmt_ctx);
    }
    if (enc_ctx) {
        avcodec_free_context(&enc_ctx);
    }
    if (rgb_frame) {
        av_frame_free(&rgb_frame);
    }
    if (packet) {
        av_packet_free(&packet);
    }
}

VTL_AppResult VTL_img_write_to_file(const char* filename, const VTL_img_data_t* img_data)
{
    if (!filename || !img_data || !img_data->current_frame) {
        return VTL_IMG_ERROR_INVALID_PARAM;
    }

    AVFormatContext *out_fmt_ctx = NULL;
    AVCodecContext *enc_ctx = NULL;
    AVStream *out_stream = NULL;
    AVPacket *packet = av_packet_alloc();
    AVFrame *rgb_frame = NULL;

    rgb_frame = av_frame_alloc();
    if (!rgb_frame) {
        cleanup_resources(NULL, NULL, NULL, packet);
        return VTL_IMG_ERROR_MEMORY;
    }

    rgb_frame->format = AV_PIX_FMT_RGB24;
    rgb_frame->width = img_data->current_frame->width;
    rgb_frame->height = img_data->current_frame->height;
    if (av_frame_get_buffer(rgb_frame, 0) < 0) {
        cleanup_resources(NULL, NULL, rgb_frame, packet);
        return VTL_IMG_ERROR_MEMORY;
    }

    SwsContext *sws_ctx = sws_getContext(
        img_data->current_frame->width, img_data->current_frame->height, img_data->current_frame->format,
        rgb_frame->width, rgb_frame->height, rgb_frame->format,
        SWS_BILINEAR, NULL, NULL, NULL
    );
    if (!sws_ctx) {
        cleanup_resources(NULL, NULL, rgb_frame, packet);
        return VTL_IMG_ERROR_MEMORY;
    }

    if (sws_scale(sws_ctx, 
                  (const uint8_t * const*)img_data->current_frame->data, img_data->current_frame->linesize,
                  0, img_data->current_frame->height,
                  rgb_frame->data, rgb_frame->linesize) < 0) {
        sws_freeContext(sws_ctx);
        cleanup_resources(NULL, NULL, rgb_frame, packet);
        return VTL_IMG_ERROR_ENCODE;
    }
    sws_freeContext(sws_ctx);

    if (avformat_alloc_output_context2(&out_fmt_ctx, NULL, NULL, filename) < 0) {
        cleanup_resources(NULL, NULL, rgb_frame, packet);
        return VTL_IMG_ERROR_ENCODE;
    }

    const AVCodec *encoder = avcodec_find_encoder(AV_CODEC_ID_PNG);
    if (!encoder) {
        cleanup_resources(out_fmt_ctx, NULL, rgb_frame, packet);
        return VTL_IMG_ERROR_ENCODE;
    }

    enc_ctx = avcodec_alloc_context3(encoder);
    if (!enc_ctx) {
        cleanup_resources(out_fmt_ctx, NULL, rgb_frame, packet);
        return VTL_IMG_ERROR_MEMORY;
    }

    enc_ctx->height = rgb_frame->height;
    enc_ctx->width = rgb_frame->width;
    enc_ctx->pix_fmt = rgb_frame->format;
    enc_ctx->time_base = (AVRational){1, 25};

    if (out_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
        enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if (avcodec_open2(enc_ctx, encoder, NULL) < 0) {
        cleanup_resources(out_fmt_ctx, enc_ctx, rgb_frame, packet);
        return VTL_IMG_ERROR_ENCODE;
    }

    out_stream = avformat_new_stream(out_fmt_ctx, NULL);
    if (!out_stream) {
        cleanup_resources(out_fmt_ctx, enc_ctx, rgb_frame, packet);
        return VTL_IMG_ERROR_MEMORY;
    }

    avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
    out_stream->time_base = enc_ctx->time_base;

    if (avio_open(&out_fmt_ctx->pb, filename, AVIO_FLAG_WRITE) < 0) {
        cleanup_resources(out_fmt_ctx, enc_ctx, rgb_frame, packet);
        return VTL_IMG_ERROR_FILE_ACCESS;
    }

    if (avformat_write_header(out_fmt_ctx, NULL) < 0) {
        cleanup_resources(out_fmt_ctx, enc_ctx, rgb_frame, packet);
        return VTL_IMG_ERROR_ENCODE;
    }

    if (avcodec_send_frame(enc_ctx, rgb_frame) < 0) {
        cleanup_resources(out_fmt_ctx, enc_ctx, rgb_frame, packet);
        return VTL_IMG_ERROR_ENCODE;
    }

    while (avcodec_receive_packet(enc_ctx, packet) == 0) {
        packet->stream_index = out_stream->index;
        if (av_interleaved_write_frame(out_fmt_ctx, packet) < 0) {
            cleanup_resources(out_fmt_ctx, enc_ctx, rgb_frame, packet);
            return VTL_IMG_ERROR_ENCODE;
        }
        av_packet_unref(packet);
    }

    av_write_trailer(out_fmt_ctx);
    cleanup_resources(out_fmt_ctx, enc_ctx, rgb_frame, packet);

    return VTL_IMG_SUCCESS;
}