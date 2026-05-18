#include "VTL_img_core.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int init_filter_graph(VTL_ImageContext* ctx, const char* filter_descr, AVFrame* input_frame)
{
    if (!ctx || !ctx->current_frame) return -1;

    char args[512];
    AVFilterInOut *inputs = avfilter_inout_alloc();
    AVFilterInOut *outputs = avfilter_inout_alloc();

    ctx->filter_graph = avfilter_graph_alloc();

    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=1/25:pixel_aspect=1/1",
             ctx->current_frame->width, ctx->current_frame->height, ctx->current_frame->format);

    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");

    avfilter_graph_create_filter(&ctx->buffersrc_ctx, buffersrc, "in", args, NULL, ctx->filter_graph);
    avfilter_graph_create_filter(&ctx->buffersink_ctx, buffersink, "out", NULL, NULL, ctx->filter_graph);

    outputs->name = av_strdup("in");
    outputs->filter_ctx = ctx->buffersrc_ctx;
    outputs->pad_idx = 0;
    outputs->next = NULL;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = ctx->buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = NULL;

    avfilter_graph_parse_ptr(ctx->filter_graph, filter_descr, &inputs, &outputs, NULL);
    avfilter_graph_config(ctx->filter_graph, NULL);

    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    return 0;
}

VTL_ImageContext* VTL_img_context_Init(void)
{
    VTL_ImageContext* ctx = (VTL_ImageContext*)malloc(sizeof(VTL_ImageContext));
    if (!ctx) return NULL;

    memset(ctx, 0, sizeof(VTL_ImageContext));
    return ctx;
}

void VTL_img_context_Cleanup(VTL_ImageContext* ctx)
{
    if (!ctx) return;

    if (ctx->format_ctx) avformat_close_input(&ctx->format_ctx);
    if (ctx->codec_ctx) avcodec_free_context(&ctx->codec_ctx);
    if (ctx->filter_graph) avfilter_graph_free(&ctx->filter_graph);
    if (ctx->sws_ctx) sws_freeContext(ctx->sws_ctx);

    free(ctx);
}

int VTL_img_LoadImage(const char* input_path, VTL_ImageContext* ctx)
{
    if (!input_path || !ctx) return -1;

    const AVCodec *decoder = NULL;
    AVPacket *packet = av_packet_alloc();
    int video_stream_index = -1;

    if (avformat_open_input(&ctx->format_ctx, input_path, NULL, NULL) < 0) {
        printf("Failed to open input file\n");
        av_packet_free(&packet);
        return -1;
    }

    if (avformat_find_stream_info(ctx->format_ctx, NULL) < 0) {
        printf("Failed to find stream info\n");
        av_packet_free(&packet);
        return -2;
    }

    video_stream_index = av_find_best_stream(
        ctx->format_ctx,
        AVMEDIA_TYPE_VIDEO,
        -1,
        -1,
        &decoder,
        0
    );

    if (video_stream_index < 0) {
        printf("Failed to find video stream\n");
        av_packet_free(&packet);
        return -3;
    }

    AVStream *in_stream = ctx->format_ctx->streams[video_stream_index];
    ctx->codec_ctx = avcodec_alloc_context3(decoder);
    if (!ctx->codec_ctx) {
        printf("Failed to allocate codec context\n");
        av_packet_free(&packet);
        return -4;
    }

    if (avcodec_parameters_to_context(ctx->codec_ctx, in_stream->codecpar) < 0) {
        printf("Failed to copy codec params\n");
        av_packet_free(&packet);
        return -5;
    }

    if (avcodec_open2(ctx->codec_ctx, decoder, NULL) < 0) {
        printf("Failed to open codec\n");
        av_packet_free(&packet);
        return -6;
    }

    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        printf("Failed to allocate frame\n");
        av_packet_free(&packet);
        return -7;
    }

    int ret = -8;
    while (av_read_frame(ctx->format_ctx, packet) >= 0) {
        if (packet->stream_index == video_stream_index) {
            if (avcodec_send_packet(ctx->codec_ctx, packet) < 0) {
                printf("Failed to send packet to decoder\n");
                break;
            }
            if (avcodec_receive_frame(ctx->codec_ctx, frame) == 0) {
                // Сохраняем первый кадр
                ctx->current_frame = av_frame_alloc();
                if (!ctx->current_frame) {
                    printf("Failed to allocate current frame\n");
                    break;
                }
                if (av_frame_ref(ctx->current_frame, frame) < 0) {
                    printf("Failed to reference frame\n");
                    break;
                }
                ret = 0;
                break;
            }
        }
        av_packet_unref(packet);
    }

    av_frame_free(&frame);
    av_packet_free(&packet);
    return ret;
}

int VTL_img_SaveImage(const char* output_path, VTL_ImageContext* ctx)
{
    if (!output_path || !ctx || !ctx->current_frame) return -1;

    AVFormatContext *out_fmt_ctx = NULL;
    AVCodecContext *enc_ctx = NULL;
    AVStream *out_stream = NULL;
    AVPacket *packet = av_packet_alloc();
    AVFrame *rgb_frame = NULL;
    int ret = 0;

    // Конвертируем в rgb24
    rgb_frame = av_frame_alloc();
    if (!rgb_frame) {
        ret = -2;
        goto cleanup;
    }

    rgb_frame->format = AV_PIX_FMT_RGB24;
    rgb_frame->width = ctx->current_frame->width;
    rgb_frame->height = ctx->current_frame->height;
    if (av_frame_get_buffer(rgb_frame, 0) < 0) {
        ret = -3;
        goto cleanup;
    }

    // Создаем контекст для конвертации
    SwsContext *sws_ctx = sws_getContext(
        ctx->current_frame->width, ctx->current_frame->height, ctx->current_frame->format,
        rgb_frame->width, rgb_frame->height, rgb_frame->format,
        SWS_BILINEAR, NULL, NULL, NULL
    );
    if (!sws_ctx) {
        ret = -4;
        goto cleanup;
    }

    // Конвертируем кадр
    if (sws_scale(sws_ctx, 
                  (const uint8_t * const*)ctx->current_frame->data, ctx->current_frame->linesize,
                  0, ctx->current_frame->height,
                  rgb_frame->data, rgb_frame->linesize) < 0) {
        ret = -5;
        sws_freeContext(sws_ctx);
        goto cleanup;
    }
    sws_freeContext(sws_ctx);

    avformat_alloc_output_context2(&out_fmt_ctx, NULL, NULL, output_path);
    const AVCodec *encoder = avcodec_find_encoder(AV_CODEC_ID_PNG);
    enc_ctx = avcodec_alloc_context3(encoder);

    enc_ctx->height = rgb_frame->height;
    enc_ctx->width = rgb_frame->width;
    enc_ctx->pix_fmt = rgb_frame->format;
    enc_ctx->time_base = (AVRational){1, 25};

    if (out_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
        enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if (avcodec_open2(enc_ctx, encoder, NULL) < 0) {
        ret = -6;
        goto cleanup;
    }

    out_stream = avformat_new_stream(out_fmt_ctx, NULL);
    avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
    out_stream->time_base = enc_ctx->time_base;

    if (avio_open(&out_fmt_ctx->pb, output_path, AVIO_FLAG_WRITE) < 0) {
        ret = -7;
        goto cleanup;
    }

    if (avformat_write_header(out_fmt_ctx, NULL) < 0) {
        ret = -8;
        goto cleanup;
    }

    // Используем сконвертированный кадр
    if (avcodec_send_frame(enc_ctx, rgb_frame) < 0) {
        ret = -9;
        goto cleanup;
    }

    while (avcodec_receive_packet(enc_ctx, packet) == 0) {
        packet->stream_index = out_stream->index;
        if (av_interleaved_write_frame(out_fmt_ctx, packet) < 0) {
            ret = -10;
            goto cleanup;
        }
        av_packet_unref(packet);
    }

    av_write_trailer(out_fmt_ctx);

cleanup:
    avcodec_free_context(&enc_ctx);
    avformat_free_context(out_fmt_ctx);
    av_frame_free(&rgb_frame);
    av_packet_free(&packet);
    return ret;
}

int VTL_img_ApplyFilter(VTL_ImageContext* ctx, const VTL_ImageFilter* filter)
{
    if (!ctx || !filter || !ctx->current_frame) return -1;

    AVFrame *filt_frame = av_frame_alloc();
    if (!filt_frame) return -2;

    int ret = 0;

    if (init_filter_graph(ctx, filter->filter_desc, ctx->current_frame) < 0) {
        ret = -3;
        goto cleanup;
    }

    ret = av_buffersrc_add_frame(ctx->buffersrc_ctx, ctx->current_frame);
    if (ret < 0) {
        ret = -4;
        goto cleanup;
    }

    if (av_buffersink_get_frame(ctx->buffersink_ctx, filt_frame) < 0) {
        ret = -5;
        goto cleanup;
    }

    // Обновляем текущий кадр
    av_frame_unref(ctx->current_frame);
    if (av_frame_ref(ctx->current_frame, filt_frame) < 0) {
        ret = -6;
        goto cleanup;
    }

cleanup:
    av_frame_free(&filt_frame);
    return ret;
}

int VTL_img_InitGPU(void)
{
    AVBufferRef* hw_device_ctx = NULL;
    int err = av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_CUDA, NULL, NULL, 0);
    if (err < 0) {
        err = av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_OPENCL, NULL, NULL, 0);
        if (err < 0) {
            fprintf(stderr, "Failed to create hardware device context\n");
            return -1;
        }
    }
    
    av_buffer_unref(&hw_device_ctx);
    return 0;
}

void VTL_img_CleanupGPU(void)
{
    // GPU контекст освобождается автоматически при очистке AVFilterGraph
    // Здесь можно добавить дополнительную очистку, если необходимо
} 