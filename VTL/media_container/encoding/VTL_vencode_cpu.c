#include <VTL_vencode.h>
#include <stdio.h>
#include <stdlib.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/hwcontext.h>
#include <libswscale/swscale.h>

extern VTL_vencode_Flags VTL_vencode_GetFlags(void);
extern int VTL_vencode_GetThreadCount(void);

VTL_AppResult VTL_vencode_CloseCPU(VTL_vencode_Context* ctx);

static const AVCodec* pick_cpu_codec(VTL_vencode_Codec c)
{
  switch (c) {
  case VTL_vencode_codec_kH264: return avcodec_find_encoder_by_name("libx264");
  case VTL_vencode_codec_kH265: return avcodec_find_encoder_by_name("libx265");
  case VTL_vencode_codec_kAV1:  return avcodec_find_encoder_by_name("libsvtav1");
  }
  return NULL;
}

VTL_vencode_Context* VTL_vencode_OpenCPU(VTL_vencode_Codec c, const char* outfile)
{
    const AVCodec* codec = pick_cpu_codec(c);
    if (!codec) return NULL;

    VTL_vencode_Context* ctx = (VTL_vencode_Context*)calloc(1, sizeof(*ctx));
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

    ctx->enc->width      = 0; 
    ctx->enc->height     = 0;
    ctx->enc->time_base  = (AVRational){1, 30};
    ctx->enc->framerate  = (AVRational){30, 1};
    ctx->enc->pix_fmt    = AV_PIX_FMT_YUV420P;
    
    /* Apply thread count from global config */
    ctx->enc->thread_count = VTL_vencode_GetThreadCount();

    return ctx;
}

VTL_AppResult VTL_vencode_EncodeAVFrameCPU(VTL_vencode_Context* ctx, AVFrame* f)
{
    if (ctx->enc->width == 0) {
        int target_w = f->width;
        int target_h = f->height;

        if (VTL_vencode_GetFlags() & VTL_VENCODE_FLAG_TG_OPTIMAL) {
            if (target_w > 1280 || target_h > 720) {
                float aspect = (float)f->width / f->height;
                if (aspect > 1.0f) {
                    target_w = 1280;
                    target_h = (int)(1280 / aspect);
                } else {
                    target_h = 720;
                    target_w = (int)(720 * aspect);
                }
            }
            target_w &= ~1;
            target_h &= ~1;

            ctx->enc->bit_rate = 2500000;
            ctx->enc->rc_max_rate = 3000000;
            ctx->enc->rc_buffer_size = 5000000;
        }

        ctx->enc->width  = target_w;
        ctx->enc->height = target_h;
        
        av_opt_set(ctx->enc->priv_data, "preset", "medium", 0);
        
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

    f->pts = ctx->frame_count++;

    AVFrame* encoding_frame = f;
    AVFrame* sws_frame = NULL;

    if (f->width != ctx->enc->width || f->height != ctx->enc->height) {
        sws_frame = av_frame_alloc();
        sws_frame->format = ctx->enc->pix_fmt;
        sws_frame->width  = ctx->enc->width;
        sws_frame->height = ctx->enc->height;
        av_frame_get_buffer(sws_frame, 0);

        struct SwsContext* sws = (struct SwsContext*)ctx->sws_ctx;
        if (!sws) {
            sws = sws_getContext(f->width, f->height, (enum AVPixelFormat)f->format,
                                 ctx->enc->width, ctx->enc->height, ctx->enc->pix_fmt,
                                 SWS_BICUBIC, NULL, NULL, NULL);
            ctx->sws_ctx = sws;
        }
        sws_scale(sws, (const uint8_t *const *)f->data, f->linesize, 0, f->height,
                  sws_frame->data, sws_frame->linesize);
        sws_frame->pts = f->pts;
        encoding_frame = sws_frame;
    }

    int ret = avcodec_send_frame(ctx->enc, encoding_frame);
    if (sws_frame) av_frame_free(&sws_frame);
    
    if (ret < 0) return VTL_res_ffmpeg_kCodecError;

    AVPacket* pkt = av_packet_alloc();
    while ((ret = avcodec_receive_packet(ctx->enc, pkt)) >= 0) {
        av_packet_rescale_ts(pkt, ctx->enc->time_base, ctx->st->time_base);
        pkt->stream_index = ctx->st->index;
        av_interleaved_write_frame(ctx->oc, pkt);
        av_packet_unref(pkt);
    }
    av_packet_free(&pkt);
    return VTL_res_kOk;
}

VTL_AppResult VTL_vencode_EncodeFrameCPU(VTL_vencode_Context* ctx,
                                        const uint8_t* data,
                                        size_t size,
                                        uint32_t w, uint32_t h)
{
    AVFrame* f = av_frame_alloc();
    if (!f) return VTL_res_kMemAllocErr;

    f->format = AV_PIX_FMT_YUV420P;
    f->width  = w;
    f->height = h;
    av_image_fill_arrays(f->data, f->linesize, data,
                        AV_PIX_FMT_YUV420P, w, h, 1);

    VTL_AppResult res = VTL_vencode_EncodeAVFrameCPU(ctx, f);
    av_frame_free(&f);
    return res;
}

VTL_AppResult VTL_vencode_CloseCPU(VTL_vencode_Context* ctx)
{
  if (!ctx) return VTL_res_kOk;
  
  if (ctx->enc && ctx->enc->codec) {
      avcodec_send_frame(ctx->enc, NULL);
      AVPacket* pkt = av_packet_alloc();
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
  
  if (ctx->sws_ctx)
      sws_freeContext((struct SwsContext*)ctx->sws_ctx);
  
  free(ctx);
  return VTL_res_kOk;
}
