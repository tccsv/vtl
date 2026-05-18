#include <VTL_vencode.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/hwcontext.h>
#include <libavutil/pixfmt.h>
#include <stdio.h>

extern int VTL_vencode_GetThreadCount(void);

static enum AVPixelFormat get_vk_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts)
{
    const enum AVPixelFormat *p;
    for (p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
        if (*p == AV_PIX_FMT_VULKAN) return *p;
    }
    return AV_PIX_FMT_NONE;
}

VTL_AppResult VTL_vencode_TranscodeFile(const char* input_file,
                                       const char* output_file,
                                       VTL_vencode_Codec codec,
                                       VTL_vencode_Api api)
{
    AVFormatContext *ifmt_ctx = NULL;
    AVCodecContext *dec_ctx = NULL;
    VTL_vencode_Context *v_ctx = NULL;
    int video_stream_index = -1;

    if (avformat_open_input(&ifmt_ctx, input_file, NULL, NULL) < 0)
        return VTL_res_kFileOpenErr;

    if (avformat_find_stream_info(ifmt_ctx, NULL) < 0) {
        avformat_close_input(&ifmt_ctx);
        return VTL_res_ffmpeg_kFormatError;
    }

    for (unsigned int i = 0; i < ifmt_ctx->nb_streams; i++) {
        if (ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }

    if (video_stream_index == -1) {
        avformat_close_input(&ifmt_ctx);
        return VTL_res_ffmpeg_kStreamError;
    }

    v_ctx = VTL_vencode_Open(codec, api, output_file);
    if (!v_ctx) {
        avformat_close_input(&ifmt_ctx);
        return VTL_res_ffmpeg_kInitError;
    }

    AVStream *in_stream = ifmt_ctx->streams[video_stream_index];
    const AVCodec *decoder = avcodec_find_decoder(in_stream->codecpar->codec_id);
    dec_ctx = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(dec_ctx, in_stream->codecpar);

    /* Apply thread count to decoder */
    dec_ctx->thread_count = VTL_vencode_GetThreadCount();

    if (api == VTL_vencode_api_kVulkan) {
        dec_ctx->hw_device_ctx = av_buffer_ref(v_ctx->hw_device);
        dec_ctx->get_format = get_vk_format;
    }

    if (avcodec_open2(dec_ctx, decoder, NULL) < 0) {
        VTL_vencode_Close(v_ctx);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&ifmt_ctx);
        return VTL_res_ffmpeg_kInitError;
    }

    AVPacket *pkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();

    while (av_read_frame(ifmt_ctx, pkt) >= 0) {
        if (pkt->stream_index == video_stream_index) {
            if (avcodec_send_packet(dec_ctx, pkt) >= 0) {
                while (avcodec_receive_frame(dec_ctx, frame) >= 0) {
                    VTL_vencode_EncodeAVFrame(v_ctx, frame);
                    av_frame_unref(frame);
                }
            }
        }
        av_packet_unref(pkt);
    }

    /* Flush decoder */
    avcodec_send_packet(dec_ctx, NULL);
    while (avcodec_receive_frame(dec_ctx, frame) >= 0) {
        VTL_vencode_EncodeAVFrame(v_ctx, frame);
        av_frame_unref(frame);
    }

    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&dec_ctx);
    VTL_vencode_Close(v_ctx);
    avformat_close_input(&ifmt_ctx);

    return VTL_res_kOk;
}
