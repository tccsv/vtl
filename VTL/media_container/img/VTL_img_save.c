#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <stdio.h>
#include <string.h>

static const char* get_fmt_name(const char* filename) {
    const char* ext = strrchr(filename, '.');
    if (!ext || strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "mjpeg";
    if (strcmp(ext, ".bmp") == 0) return "bmp";
    if (strcmp(ext, ".png") == 0) return "png";
    return "mjpeg";
}

static int open_output_context(const char* filename, AVFormatContext** fmt_ctx, const char** fmt_name) {
    *fmt_name = get_fmt_name(filename);
    if (strcmp(*fmt_name, "png") == 0) {
        AVOutputFormat* ofmt = av_guess_format("png", NULL, NULL);
        if (!ofmt) {
            // fallback на jpeg
            *fmt_name = "mjpeg";
            return avformat_alloc_output_context2(fmt_ctx, NULL, *fmt_name, filename);
        }
        return avformat_alloc_output_context2(fmt_ctx, ofmt, NULL, filename);
    }
    return avformat_alloc_output_context2(fmt_ctx, NULL, *fmt_name, filename);
}

static int setup_codec_stream(AVFormatContext* fmt_ctx, AVCodecContext** codec_ctx, AVStream** stream, AVFrame* frame) {
    const AVCodec* codec = avcodec_find_encoder(fmt_ctx->oformat->video_codec);
    if (!codec) return AVERROR_ENCODER_NOT_FOUND;
    *stream = avformat_new_stream(fmt_ctx, codec);
    if (!*stream) return AVERROR_UNKNOWN;
    *codec_ctx = avcodec_alloc_context3(codec);
    if (!*codec_ctx) return AVERROR(ENOMEM);
    (*codec_ctx)->codec_id = fmt_ctx->oformat->video_codec;
    (*codec_ctx)->width = frame->width;
    (*codec_ctx)->height = frame->height;
    (*codec_ctx)->pix_fmt = frame->format;
    (*codec_ctx)->time_base = (AVRational){1, 25};
    if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        (*codec_ctx)->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    int ret = avcodec_open2(*codec_ctx, codec, NULL);
    if (ret < 0) { avcodec_free_context(codec_ctx); *codec_ctx = NULL; return ret; }
    ret = avcodec_parameters_from_context((*stream)->codecpar, *codec_ctx);
    return ret;
}

static int write_frame_and_trailer(AVFormatContext* fmt_ctx, AVCodecContext* codec_ctx, AVStream* stream, AVFrame* frame, const char* filename) {
    int ret = avio_open(&fmt_ctx->pb, filename, AVIO_FLAG_WRITE);
    if (ret < 0) return ret;
    ret = avformat_write_header(fmt_ctx, NULL);
    if (ret < 0) return ret;
    AVPacket pkt = {0};
    av_init_packet(&pkt);
    ret = avcodec_send_frame(codec_ctx, frame);
    if (ret < 0) return ret;
    ret = avcodec_receive_packet(codec_ctx, &pkt);
    if (ret < 0) return ret;
    pkt.stream_index = stream->index;
    ret = av_write_frame(fmt_ctx, &pkt);
    av_packet_unref(&pkt);
    av_write_trailer(fmt_ctx);
    return ret;
}

static AVFrame* convert_to_yuvj420p(AVFrame* src, int width, int height) {
    struct SwsContext* sws_ctx = sws_getContext(width, height, src->format, width, height, AV_PIX_FMT_YUVJ420P, SWS_BILINEAR, NULL, NULL, NULL);
    if (!sws_ctx) return NULL;
    AVFrame* yuv = av_frame_alloc();
    yuv->format = AV_PIX_FMT_YUVJ420P;
    yuv->width = width;
    yuv->height = height;
    av_frame_get_buffer(yuv, 32);
    sws_scale(sws_ctx, (const uint8_t* const*)src->data, src->linesize, 0, height, yuv->data, yuv->linesize);
    sws_freeContext(sws_ctx);
    return yuv;
}

int VTL_img_save(const char* filename, AVFrame* frame) {
    AVFormatContext* fmt_ctx = NULL;
    AVCodecContext* codec_ctx = NULL;
    AVStream* stream = NULL;
    const char* fmt_name = NULL;
    int ret = open_output_context(filename, &fmt_ctx, &fmt_name);
    if (!fmt_ctx || ret < 0) return AVERROR_UNKNOWN;
    AVFrame* frame_to_save = frame;
    if (strcmp(fmt_name, "mjpeg") == 0 && frame->format == AV_PIX_FMT_RGB24) {
        AVFrame* yuv = convert_to_yuvj420p(frame, frame->width, frame->height);
        if (!yuv) { avformat_free_context(fmt_ctx); return -300; }
        frame_to_save = yuv;
    }
    ret = setup_codec_stream(fmt_ctx, &codec_ctx, &stream, frame_to_save);
    if (ret < 0) { avcodec_free_context(&codec_ctx); avformat_free_context(fmt_ctx); if (frame_to_save != frame) av_frame_free(&frame_to_save); return ret; }
    ret = write_frame_and_trailer(fmt_ctx, codec_ctx, stream, frame_to_save, filename);
    avcodec_free_context(&codec_ctx);
    if (fmt_ctx) {
        if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE) && fmt_ctx->pb)
            avio_closep(&fmt_ctx->pb);
        avformat_free_context(fmt_ctx);
    }
    if (frame_to_save != frame) av_frame_free(&frame_to_save);
    return ret;
} 