#include <VTL_vencode.h>
#include <VTL/media_container/encoding/VTL_vencode_vk_helper.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

static const AVCodec* pick_vk_codec(VTL_vencode_Codec c)
{
    switch (c) {
    case VTL_vencode_codec_kH264: return avcodec_find_encoder_by_name("h264_vulkan");
    case VTL_vencode_codec_kH265: return avcodec_find_encoder_by_name("hevc_vulkan");
    case VTL_vencode_codec_kAV1:  return avcodec_find_encoder_by_name("av1_vulkan");
    }
    return NULL;
}

VTL_AppResult VTL_vencode_CloseVulkan(VTL_vencode_Context *ctx);

VTL_vencode_Context* VTL_vencode_OpenVulkan(VTL_vencode_Codec c, const char* outfile)
{
    VkApplicationInfo app = {
           .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
           .pApplicationName = "libvencode",
           .applicationVersion = VK_MAKE_VERSION(1,0,0),
           .pEngineName = "libvencode",
           .engineVersion = VK_MAKE_VERSION(1,0,0),
           .apiVersion = VK_API_VERSION_1_3
       };
    VkInstanceCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = NULL,
    };
    VkInstance inst;
    if (vkCreateInstance(&ci, NULL, &inst) != VK_SUCCESS) return NULL;

    uint32_t n = 0;
    vkEnumeratePhysicalDevices(inst, &n, NULL);
    if (n == 0) {
        vkDestroyInstance(inst, NULL);
        return NULL;
    }
    VkPhysicalDevice *pdevs = malloc(n * sizeof(*pdevs));
    vkEnumeratePhysicalDevices(inst, &n, pdevs);

    VTL_AppResult cap = vk_check_video_caps(pdevs[0], c, VK_VIDEO_DIR_ENCODE);
    free(pdevs);
    vkDestroyInstance(inst, NULL);        
    if (cap != VTL_res_kOk) {
        fprintf(stderr, "vulkan: no video-encode support for codec %d\n", c);
        return NULL;
    }
    const AVCodec *codec = pick_vk_codec(c);
    if (!codec) return NULL;

    VTL_vencode_Context *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) return NULL;

    if (avformat_alloc_output_context2(&ctx->oc, NULL, NULL, outfile) < 0) {
        free(ctx);
        return NULL;
    }

    ctx->enc = avcodec_alloc_context3(codec);
    if (!ctx->enc) {
        avformat_free_context(ctx->oc);
        free(ctx);
        return NULL;
    }

    ctx->enc->width   = 0;
    ctx->enc->height  = 0;
    ctx->enc->time_base = (AVRational){1,30};
    ctx->enc->framerate = (AVRational){30,1};
    ctx->enc->pix_fmt   = AV_PIX_FMT_VULKAN;

    if (av_hwdevice_ctx_create(&ctx->hw_device, AV_HWDEVICE_TYPE_VULKAN,
                            NULL, NULL, 0) < 0)
    {
        VTL_vencode_CloseVulkan(ctx);
        return NULL;
    }
    ctx->enc->hw_device_ctx = av_buffer_ref(ctx->hw_device);

    return ctx;
}

static int vk_transfer_frame(AVCodecContext *enc,
                                   AVFrame *hw_frame,
                                   const uint8_t *data,
                                   uint32_t w, uint32_t h);

VTL_AppResult VTL_vencode_EncodeAVFrameVulkan(VTL_vencode_Context *ctx, AVFrame* frame)
{
    if (ctx->enc->width == 0) {
        ctx->enc->width  = frame->width;
        ctx->enc->height = frame->height;
        
        AVBufferRef *hw_frames_ref = av_hwframe_ctx_alloc(ctx->hw_device);
        AVHWFramesContext *hwfc = (AVHWFramesContext*)hw_frames_ref->data;
        hwfc->format    = AV_PIX_FMT_VULKAN;
        hwfc->sw_format = AV_PIX_FMT_YUV420P;
        hwfc->width     = frame->width;
        hwfc->height    = frame->height;
        if (av_hwframe_ctx_init(hw_frames_ref) < 0) 
            return VTL_res_ffmpeg_kInitError;
        ctx->enc->hw_frames_ctx = hw_frames_ref;
        
        if (avcodec_open2(ctx->enc, ctx->enc->codec, NULL) < 0) 
            return VTL_res_ffmpeg_kInitError;
        
        ctx->st = avformat_new_stream(ctx->oc, NULL);
        if (!ctx->st) return VTL_res_ffmpeg_kStreamError;
        avcodec_parameters_from_context(ctx->st->codecpar, ctx->enc);
        
        if (!(ctx->oc->oformat->flags & AVFMT_NOFILE))
            if (avio_open(&ctx->oc->pb, ctx->oc->url, AVIO_FLAG_WRITE) < 0) 
                return VTL_res_ffmpeg_kIOError;
        
        if (avformat_write_header(ctx->oc, NULL) < 0) 
            return VTL_res_ffmpeg_kFormatError;
    }

    frame->pts = ctx->frame_count++;

    int ret = avcodec_send_frame(ctx->enc, frame);
    if (ret < 0) return VTL_res_ffmpeg_kCodecError;

    AVPacket *pkt = av_packet_alloc();
    while (ret >= 0) {
        ret = avcodec_receive_packet(ctx->enc, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        if (ret < 0) { av_packet_free(&pkt); return VTL_res_ffmpeg_kCodecError; }
        av_packet_rescale_ts(pkt, ctx->enc->time_base, ctx->st->time_base);
        pkt->stream_index = ctx->st->index;
        av_interleaved_write_frame(ctx->oc, pkt);
        av_packet_unref(pkt);
    }
    av_packet_free(&pkt);
    return VTL_res_kOk;
}

VTL_AppResult VTL_vencode_EncodeFrameVulkan(VTL_vencode_Context *ctx,
                                         const uint8_t *data,
                                         size_t size,
                                         uint32_t w, uint32_t h)
{
    if (!ctx || !data) return VTL_res_kInvalidParamErr;

    AVFrame *hw_frame = av_frame_alloc();
    if (!hw_frame) return VTL_res_kMemAllocErr;
    
    if (ctx->enc->width == 0) {
        /* Temporary frame for init */
        AVFrame* tmp = av_frame_alloc();
        tmp->width = w; tmp->height = h;
        VTL_vencode_EncodeAVFrameVulkan(ctx, tmp);
        av_frame_free(&tmp);
    }

    hw_frame->format = AV_PIX_FMT_VULKAN;
    hw_frame->width  = w;
    hw_frame->height = h;
    
    if (av_hwframe_get_buffer(ctx->enc->hw_frames_ctx, hw_frame, 0) < 0) {
        av_frame_free(&hw_frame);
        return VTL_res_ffmpeg_kMemoryError;
    }

    if (vk_transfer_frame(ctx->enc, hw_frame, data, w, h) < 0) {
        av_frame_free(&hw_frame);
        return VTL_res_ffmpeg_kConversionError;
    }

    VTL_AppResult res = VTL_vencode_EncodeAVFrameVulkan(ctx, hw_frame);
    av_frame_free(&hw_frame);
    return res;
}

VTL_AppResult VTL_vencode_CloseVulkan(VTL_vencode_Context *ctx)
{
    if (!ctx) return VTL_res_kOk;

    if (ctx->enc && ctx->enc->codec) {
        avcodec_send_frame(ctx->enc, NULL);
        AVPacket *pkt = av_packet_alloc();
        int ret;
        while ((ret = avcodec_receive_packet(ctx->enc, pkt)) >= 0) {
            av_packet_rescale_ts(pkt, ctx->enc->time_base, ctx->st->time_base);
            pkt->stream_index = ctx->st->index;
            av_interleaved_write_frame(ctx->oc, pkt);
            av_packet_unref(pkt);
        }
        av_packet_free(&pkt);
    }

    if (ctx->oc) {
        av_write_trailer(ctx->oc);
        avio_closep(&ctx->oc->pb);
        avformat_free_context(ctx->oc);
    }

    if (ctx->enc)
        avcodec_free_context(&ctx->enc);
    
    if (ctx->hw_device)
        av_buffer_unref(&ctx->hw_device);
    
    free(ctx);
    return VTL_res_kOk;
}

static int vk_transfer_frame(AVCodecContext *enc,
                            AVFrame *hw_frame,
                            const uint8_t *data,
                            uint32_t w, uint32_t h)
{
    AVFrame *tmp = av_frame_alloc();
    if (!tmp) return -1;
    tmp->format = AV_PIX_FMT_YUV420P;
    tmp->width  = w;
    tmp->height = h;
    av_image_fill_arrays(tmp->data, tmp->linesize, data,
                        AV_PIX_FMT_YUV420P, w, h, 1);

    int ret = av_hwframe_transfer_data(hw_frame, tmp, 0);
    av_frame_free(&tmp);
    return ret;
}

#ifdef __cplusplus
}
#endif
