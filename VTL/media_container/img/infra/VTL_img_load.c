#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <stdio.h>

static int open_input(const char* filename, AVFormatContext** fmt_ctx) {
    int ret = avformat_open_input(fmt_ctx, filename, NULL, NULL);
    if (ret < 0) return ret;
    ret = avformat_find_stream_info(*fmt_ctx, NULL);
    if (ret < 0) { avformat_close_input(fmt_ctx); *fmt_ctx = NULL; }
    return ret;
}

static int find_video_stream(AVFormatContext* fmt_ctx) {
    return av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
}

static int open_codec_context(AVFormatContext* fmt_ctx, int stream_idx, AVCodecContext** codec_ctx) {
    AVStream* stream = fmt_ctx->streams[stream_idx];
    AVCodecParameters* codecpar = stream->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) return AVERROR_DECODER_NOT_FOUND;
    *codec_ctx = avcodec_alloc_context3(codec);
    if (!*codec_ctx) return AVERROR(ENOMEM);
    avcodec_parameters_to_context(*codec_ctx, codecpar);
    int ret = avcodec_open2(*codec_ctx, codec, NULL);
    if (ret < 0) { avcodec_free_context(codec_ctx); *codec_ctx = NULL; }
    return ret;
}

static int decode_first_frame(AVFormatContext* fmt_ctx, AVCodecContext* codec_ctx, int stream_idx, AVFrame** out_frame) {
    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    int got_frame = 0, ret = 0;
    while (av_read_frame(fmt_ctx, pkt) >= 0) {
        if (pkt->stream_index == stream_idx) {
            ret = avcodec_send_packet(codec_ctx, pkt);
            if (ret < 0) break;
            while (ret >= 0) {
                ret = avcodec_receive_frame(codec_ctx, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
                if (ret < 0) break;
                got_frame = 1;
                break;
            }
        }
        av_packet_unref(pkt);
        if (got_frame) break;
    }
    av_packet_free(&pkt);
    if (!got_frame) { av_frame_free(&frame); return -1; }
    *out_frame = frame;
    return 0;
}

static AVFrame* convert_to_rgb24(AVFrame* src, int width, int height, enum AVPixelFormat src_fmt) {
    if (src_fmt == AV_PIX_FMT_RGB24) return av_frame_clone(src);
    struct SwsContext* sws_ctx = sws_getContext(width, height, src_fmt, width, height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
    if (!sws_ctx) return NULL;
    AVFrame* rgb = av_frame_alloc();
    rgb->format = AV_PIX_FMT_RGB24;
    rgb->width = width;
    rgb->height = height;
    av_frame_get_buffer(rgb, 32);
    sws_scale(sws_ctx, (const uint8_t* const*)src->data, src->linesize, 0, height, rgb->data, rgb->linesize);
    sws_freeContext(sws_ctx);
    return rgb;
}

int VTL_img_load(const char* filename, AVFrame** out_frame, int* width, int* height, enum AVPixelFormat* pix_fmt) {
    AVFormatContext* fmt_ctx = NULL;
    AVCodecContext* codec_ctx = NULL;
    AVFrame* frame = NULL;
    int ret = open_input(filename, &fmt_ctx);
    if (ret < 0) return ret;
    int stream_idx = find_video_stream(fmt_ctx);
    if (stream_idx < 0) { avformat_close_input(&fmt_ctx); return stream_idx; }
    ret = open_codec_context(fmt_ctx, stream_idx, &codec_ctx);
    if (ret < 0) { avformat_close_input(&fmt_ctx); return ret; }
    ret = decode_first_frame(fmt_ctx, codec_ctx, stream_idx, &frame);
    if (ret < 0) {
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return ret;
    }
    AVFrame* rgb = convert_to_rgb24(frame, frame->width, frame->height, frame->format);
    if (!rgb) {
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return -200;
    }
    *out_frame = rgb;
    *width = rgb->width;
    *height = rgb->height;
    *pix_fmt = rgb->format;
    av_frame_free(&frame);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    return 0;
} 