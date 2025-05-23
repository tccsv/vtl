#include <VTL/media_container/audio/VTL_audio_data.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libavutil/mem.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/frame.h>
#include <libavutil/opt.h>


static enum AVCodecID VTL_codec_to_avcodec_id(VTL_audio_Codec codec) {
    switch (codec) {
        case VTL_audio_codec_kAAC:    return AV_CODEC_ID_AAC;
        case VTL_audio_codec_kVorbis: return AV_CODEC_ID_VORBIS;
        case VTL_audio_codec_kOpus:   return AV_CODEC_ID_OPUS;
        case VTL_audio_codec_kFLAC:   return AV_CODEC_ID_FLAC;
        default:                      return AV_CODEC_ID_NONE;
    }
}

static AVCodecContext* VTL_audio_prepare_encoder(const VTL_audio_Params *p_new, const AVCodec **out_codec) {
    enum AVCodecID codec_id = VTL_codec_to_avcodec_id(p_new->codec);
    if (codec_id == AV_CODEC_ID_NONE) return NULL;
    const AVCodec *codec = avcodec_find_encoder(codec_id);
    if (!codec) return NULL;
    AVCodecContext *ctx = avcodec_alloc_context3(codec);
    if (!ctx) return NULL;
    ctx->bit_rate = p_new->bitrate;
    ctx->sample_rate = p_new->sample_rate;
    ctx->ch_layout.nb_channels = p_new->num_channels;
    av_channel_layout_default(&ctx->ch_layout, ctx->ch_layout.nb_channels);
    ctx->sample_fmt = codec->sample_fmts ? codec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
    if (avcodec_open2(ctx, codec, NULL) < 0) {
        avcodec_free_context(&ctx);
        return NULL;
    }
    if (out_codec) *out_codec = codec;
    return ctx;
}

static AVFrame* VTL_audio_prepare_frame(AVCodecContext *ctx, VTL_audio_Data *src) {
    AVFrame *frame = av_frame_alloc();
    if (!frame) return NULL;
    frame->nb_samples = ctx->frame_size > 0 ? ctx->frame_size : 1024;
    frame->format = ctx->sample_fmt;
    frame->ch_layout = ctx->ch_layout;
    frame->sample_rate = ctx->sample_rate;
    if (av_frame_get_buffer(frame, 0) < 0) {
        av_frame_free(&frame);
        return NULL;
    }
    size_t copy_bytes = src->data_size < frame->linesize[0] ? src->data_size : frame->linesize[0];
    memcpy(frame->data[0], src->data, copy_bytes);
    return frame;
}

static AVPacket* VTL_audio_encode_frame(AVCodecContext *ctx, AVFrame *frame) {
    AVPacket *pkt = av_packet_alloc();
    if (!pkt) return NULL;
    int ret = avcodec_send_frame(ctx, frame);
    if (ret < 0) {
        av_packet_free(&pkt);
        return NULL;
    }
    ret = avcodec_receive_packet(ctx, pkt);
    if (ret < 0) {
        av_packet_free(&pkt);
        return NULL;
    }
    return pkt;
}

static void VTL_audio_replace_buffer(VTL_audio_Data **pp_audio_part, AVPacket *pkt) {
    VTL_audio_Data *out = (VTL_audio_Data*)malloc(sizeof(VTL_audio_Data));
    out->data = (char*)malloc(pkt->size);
    memcpy(out->data, pkt->data, pkt->size);
    out->data_size = pkt->size;
    free((*pp_audio_part)->data);
    free(*pp_audio_part);
    *pp_audio_part = out;
}

VTL_AppResult VTL_audio_Data_Encode(VTL_audio_Data **pp_audio_part, const VTL_audio_Params *p_old, const VTL_audio_Params *p_new) {
    if (!pp_audio_part || !*pp_audio_part || !p_new) return VTL_res_video_fs_r_kMissingFileErr;
    const AVCodec *codec = NULL;
    AVCodecContext *ctx = VTL_audio_prepare_encoder(p_new, &codec);
    if (!ctx) return VTL_res_video_fs_r_kMissingFileErr;
    AVFrame *frame = VTL_audio_prepare_frame(ctx, *pp_audio_part);
    if (!frame) {
        avcodec_free_context(&ctx);
        return VTL_res_video_fs_r_kMissingFileErr;
    }
    AVPacket *pkt = VTL_audio_encode_frame(ctx, frame);
    if (!pkt) {
        av_frame_free(&frame);
        avcodec_free_context(&ctx);
        return VTL_res_video_fs_r_kMissingFileErr;
    }
    VTL_audio_replace_buffer(pp_audio_part, pkt);
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&ctx);
    return VTL_res_kOk;
}

VTL_AppResult VTL_audio_params_BitrateSet(VTL_audio_Params* p_params, const VTL_audio_Bitrate bitrate)
{
    if (!p_params || bitrate == 0)
        return VTL_res_video_fs_r_kMissingFileErr;
    p_params->bitrate = bitrate;
    return VTL_res_kOk;
}

VTL_AppResult VTL_audio_params_CodecSet(VTL_audio_Params* p_params, const VTL_audio_Codec codec)
{
    if (!p_params)
        return VTL_res_video_fs_r_kMissingFileErr;
    if (codec < VTL_audio_codec_kAAC || codec > VTL_audio_codec_kFLAC)
        return VTL_res_video_fs_r_kMissingFileErr;
    p_params->codec = codec;
    return VTL_res_kOk;
}

VTL_AppResult VTL_audio_params_SampleRateSet(VTL_audio_Params* p_params, const VTL_audio_SampleRate sample_rate)
{
    if (!p_params || sample_rate < 8000 || sample_rate > 192000)
        return VTL_res_video_fs_r_kMissingFileErr;
    p_params->sample_rate = sample_rate;
    return VTL_res_kOk;
}

VTL_AppResult VTL_audio_params_NumChannelsSet(VTL_audio_Params* p_params, const VTL_audio_NumChannels numChannels)
{
    if (!p_params || numChannels < 1 || numChannels > 8)
        return VTL_res_video_fs_r_kMissingFileErr;
    p_params->num_channels = numChannels;
    return VTL_res_kOk;
}

VTL_AppResult VTL_audio_params_VolumeSet(VTL_audio_Params* p_params, const VTL_audio_Volume volume)
{
    if (!p_params || volume > 100)
        return VTL_res_video_fs_r_kMissingFileErr;
    p_params->volume = volume;
    return VTL_res_kOk;
}