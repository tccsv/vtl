#include "VTL_video_editor.h"
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/audio_fifo.h>

typedef struct AudioProcessingContext {
    SwrContext* swr_ctx;
    AVAudioFifo* fifo;
    AVFrame* tmp_frame;
} AudioProcessingContext;


const VTL_AudioParams VTL_AUDIO_PRESET_AAC_DEFAULT = {
    .codec_id = AV_CODEC_ID_AAC,
    .bit_rate = 128000,
    .sample_rate = 44100,
    .channels = 2,
    .channel_layout = AV_CH_LAYOUT_STEREO,
    .volume = 1.0f,
    .profile = FF_PROFILE_AAC_LOW,
    .sample_fmt = AV_SAMPLE_FMT_FLTP
};

const VTL_AudioParams VTL_AUDIO_PRESET_MP3_HQ = {
    .codec_id = AV_CODEC_ID_MP3,
    .bit_rate = 256000,
    .sample_rate = 48000,
    .channels = 2,
    .channel_layout = AV_CH_LAYOUT_STEREO,
    .volume = 1.0f,
    .profile = FF_PROFILE_UNKNOWN,
    .sample_fmt = AV_SAMPLE_FMT_S16P
};

static int init_audio_encoder(AVCodecContext** enc_ctx, const VTL_AudioParams* params) {
    const AVCodec* codec = avcodec_find_encoder(params->codec_id);
    if (!codec) {
        av_log(NULL, AV_LOG_ERROR, "Failed to find encoder for codec %d\n", params->codec_id);
        return AVERROR_ENCODER_NOT_FOUND;
    }

    *enc_ctx = avcodec_alloc_context3(codec);
    if (!*enc_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Failed to allocate encoder context\n");
        return AVERROR(ENOMEM);
    }

    (*enc_ctx)->bit_rate = params->bit_rate;
    (*enc_ctx)->sample_rate = params->sample_rate;
    av_channel_layout_from_mask(&(*enc_ctx)->ch_layout, params->channel_layout);
    (*enc_ctx)->sample_fmt = params->sample_fmt;
    (*enc_ctx)->profile = params->profile;
    
    if (params->codec_id == AV_CODEC_ID_AAC) {
        (*enc_ctx)->frame_size = 1024;
    } else if (params->codec_id == AV_CODEC_ID_MP3) {
        (*enc_ctx)->frame_size = 1152;
    }

    if (avcodec_open2(*enc_ctx, codec, NULL) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to open encoder\n");
        avcodec_free_context(enc_ctx);
        return AVERROR_ENCODER_NOT_FOUND;
    }
    
    return 0;
}

static int init_audio_processing(AudioProcessingContext* ctx, 
                               AVCodecContext* dec_ctx, 
                               AVCodecContext* enc_ctx) {
    ctx->swr_ctx = swr_alloc();
    if (!ctx->swr_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Failed to allocate resampler\n");
        return AVERROR(ENOMEM);
    }

    av_opt_set_int(ctx->swr_ctx, "in_sample_rate", dec_ctx->sample_rate, 0);
    av_opt_set_int(ctx->swr_ctx, "out_sample_rate", enc_ctx->sample_rate, 0);
    av_opt_set_chlayout(ctx->swr_ctx, "in_chlayout", &dec_ctx->ch_layout, 0);
    av_opt_set_chlayout(ctx->swr_ctx, "out_chlayout", &enc_ctx->ch_layout, 0);
    av_opt_set_sample_fmt(ctx->swr_ctx, "in_sample_fmt", dec_ctx->sample_fmt, 0);
    av_opt_set_sample_fmt(ctx->swr_ctx, "out_sample_fmt", enc_ctx->sample_fmt, 0);

    av_opt_set_int(ctx->swr_ctx, "force_resampling", 1, 0);
    av_opt_set_int(ctx->swr_ctx, "compensation_distance", 0, 0);

    if (swr_init(ctx->swr_ctx) < 0) {
        char error_buf[256];
        av_strerror(AVERROR(EINVAL), error_buf, sizeof(error_buf));
        av_log(NULL, AV_LOG_ERROR, "Failed to initialize resampler: %s\n", error_buf);
        swr_free(&ctx->swr_ctx);
        return AVERROR(EINVAL);
    }

    ctx->fifo = av_audio_fifo_alloc(enc_ctx->sample_fmt, 
                                   enc_ctx->ch_layout.nb_channels, 
                                   1024 * 10);
    if (!ctx->fifo) {
        av_log(NULL, AV_LOG_ERROR, "Failed to allocate audio FIFO\n");
        swr_free(&ctx->swr_ctx);
        return AVERROR(ENOMEM);
    }

    ctx->tmp_frame = av_frame_alloc();
    if (!ctx->tmp_frame) {
        av_log(NULL, AV_LOG_ERROR, "Failed to allocate temporary frame\n");
        av_audio_fifo_free(ctx->fifo);
        swr_free(&ctx->swr_ctx);
        return AVERROR(ENOMEM);
    }

    ctx->tmp_frame->format = enc_ctx->sample_fmt;
    ctx->tmp_frame->sample_rate = enc_ctx->sample_rate;
    ctx->tmp_frame->ch_layout = enc_ctx->ch_layout;

    return 0;
}

static void free_audio_processing(AudioProcessingContext* ctx) {
    if (ctx->swr_ctx) {
        swr_close(ctx->swr_ctx);
        swr_free(&ctx->swr_ctx);
    }
    if (ctx->fifo) av_audio_fifo_free(ctx->fifo);
    if (ctx->tmp_frame) av_frame_free(&ctx->tmp_frame);
}

static int process_audio_frame(AVFrame* frame, 
                             const VTL_AudioParams* params,
                             AudioProcessingContext* ctx,
                             AVCodecContext* enc_ctx,
                             AVFormatContext* out_ctx,
                             AVStream* out_stream) {
    if (params->volume != 1.0f) {
        if (frame->format == AV_SAMPLE_FMT_FLT || frame->format == AV_SAMPLE_FMT_FLTP) {
            float* data = (float*)frame->data[0];
            for (int i = 0; i < frame->nb_samples * frame->ch_layout.nb_channels; i++) {
                data[i] = av_clipf(data[i] * params->volume, -1.0f, 1.0f);
            }
        } else if (frame->format == AV_SAMPLE_FMT_S16 || frame->format == AV_SAMPLE_FMT_S16P) {
            int16_t* data = (int16_t*)frame->data[0];
            for (int i = 0; i < frame->nb_samples * frame->ch_layout.nb_channels; i++) {
                data[i] = av_clip_int16(data[i] * params->volume);
            }
        }
    }

    if (ctx->swr_ctx) {
        av_frame_unref(ctx->tmp_frame);
        
        int out_samples = av_rescale_rnd(
            swr_get_delay(ctx->swr_ctx, frame->sample_rate) + frame->nb_samples,
            enc_ctx->sample_rate,
            frame->sample_rate,
            AV_ROUND_UP
        );

        ctx->tmp_frame->nb_samples = out_samples;
        ctx->tmp_frame->format = enc_ctx->sample_fmt;
        ctx->tmp_frame->sample_rate = enc_ctx->sample_rate;
        ctx->tmp_frame->ch_layout = enc_ctx->ch_layout;

        if (av_frame_get_buffer(ctx->tmp_frame, 0) < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to allocate tmp frame buffers (samples: %d)\n", out_samples);
            
            ctx->tmp_frame->nb_samples = enc_ctx->frame_size;
            if (av_frame_get_buffer(ctx->tmp_frame, 0) < 0) {
                return AVERROR(ENOMEM);
            }
        }

        int ret = swr_convert_frame(ctx->swr_ctx, ctx->tmp_frame, frame);
        if (ret < 0) {
            char error_buf[256];
            av_strerror(ret, error_buf, sizeof(error_buf));
            av_log(NULL, AV_LOG_WARNING, "swr_convert_frame failed: %s, trying manual conversion\n", error_buf);
            
            ret = swr_convert(
                ctx->swr_ctx,
                ctx->tmp_frame->data, ctx->tmp_frame->nb_samples,
                (const uint8_t**)frame->data, frame->nb_samples
            );
            
            if (ret < 0) {
                av_strerror(ret, error_buf, sizeof(error_buf));
                av_log(NULL, AV_LOG_ERROR, "Manual conversion failed: %s\n", error_buf);
                return ret;
            }
            
            ctx->tmp_frame->nb_samples = ret;
        }
        
        frame = ctx->tmp_frame;
    }

    if (av_audio_fifo_write(ctx->fifo, (void**)frame->data, frame->nb_samples) < frame->nb_samples) {
        av_log(NULL, AV_LOG_ERROR, "Could not write to FIFO\n");
        return AVERROR(ENOMEM);
    }

    while (av_audio_fifo_size(ctx->fifo) >= enc_ctx->frame_size) {
        AVFrame* enc_frame = av_frame_alloc();
        if (!enc_frame) return AVERROR(ENOMEM);

        enc_frame->nb_samples = FFMIN(enc_ctx->frame_size, av_audio_fifo_size(ctx->fifo));
        enc_frame->ch_layout = enc_ctx->ch_layout;
        enc_frame->format = enc_ctx->sample_fmt;
        enc_frame->sample_rate = enc_ctx->sample_rate;

        if (av_frame_get_buffer(enc_frame, 0) < 0) {
            av_frame_free(&enc_frame);
            return AVERROR(ENOMEM);
        }

        if (av_audio_fifo_read(ctx->fifo, (void**)enc_frame->data, enc_frame->nb_samples) < enc_frame->nb_samples) {
            av_frame_free(&enc_frame);
            return AVERROR(EINVAL);
        }

        AVPacket* pkt = av_packet_alloc();
        int ret = avcodec_send_frame(enc_ctx, enc_frame);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error sending frame to encoder\n");
            av_frame_free(&enc_frame);
            av_packet_free(&pkt);
            return ret;
        }

        int encoding_done = 0;
        while (!encoding_done && ret >= 0) {
            ret = avcodec_receive_packet(enc_ctx, pkt);
            if (ret == AVERROR(EAGAIN)) {
                encoding_done = 1;
            } else if (ret == AVERROR_EOF) {
                encoding_done = 1;
            } else if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Error encoding audio frame\n");
                av_frame_free(&enc_frame);
                av_packet_free(&pkt);
                return ret;
            } else {
                av_packet_rescale_ts(pkt, enc_ctx->time_base, out_stream->time_base);
                pkt->stream_index = out_stream->index;
                if (av_interleaved_write_frame(out_ctx, pkt) < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Error writing audio packet\n");
                    av_frame_free(&enc_frame);
                    av_packet_free(&pkt);
                    return AVERROR(EIO);
                }

                av_packet_unref(pkt);
            }
        }

        av_frame_free(&enc_frame);
        av_packet_free(&pkt);
    }

    return 0;
}

static int process_remaining_audio(AudioProcessingContext* ap_ctx,
                                 AVCodecContext* audio_enc_ctx,
                                 AVFormatContext* out_ctx,
                                 int audio_stream_idx,
                                 AVPacket* pkt) {
    int ret = 0;
    AVFrame* enc_frame = av_frame_alloc();
    if (!enc_frame) {
        return AVERROR(ENOMEM);
    }

    enc_frame->nb_samples = av_audio_fifo_size(ap_ctx->fifo);
    enc_frame->ch_layout = audio_enc_ctx->ch_layout;
    enc_frame->format = audio_enc_ctx->sample_fmt;
    enc_frame->sample_rate = audio_enc_ctx->sample_rate;

    if (av_frame_get_buffer(enc_frame, 0) < 0) {
        av_frame_free(&enc_frame);
        return -1;
    }

    if (av_audio_fifo_read(ap_ctx->fifo, (void**)enc_frame->data, enc_frame->nb_samples) < enc_frame->nb_samples) {
        av_frame_free(&enc_frame);
        return -1;
    }

    ret = avcodec_send_frame(audio_enc_ctx, enc_frame);
    av_frame_free(&enc_frame);
    
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error sending final audio frame to encoder\n");
        return ret;
    }

    int encoding_done = 0;
    while (!encoding_done && ret >= 0) {
        ret = avcodec_receive_packet(audio_enc_ctx, pkt);
        if (ret == AVERROR(EAGAIN)) {
            encoding_done = 1;
        } else if (ret == AVERROR_EOF) {
            encoding_done = 1;
        } else if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error encoding final audio frame\n");
            return ret;
        } else {
            av_packet_rescale_ts(pkt, audio_enc_ctx->time_base, out_ctx->streams[audio_stream_idx]->time_base);
            pkt->stream_index = out_ctx->streams[audio_stream_idx]->index;
            if (av_interleaved_write_frame(out_ctx, pkt) < 0) {
                av_log(NULL, AV_LOG_ERROR, "Error writing final audio packet\n");
                av_packet_unref(pkt);
                return AVERROR(EIO);
            }

            av_packet_unref(pkt);
        }
    }

    return 0;
}

VTL_AppResult VTL_process_video(
    const char* input_path,
    const char* output_path,
    const VTL_AudioParams* audio_params) {
    
    AVFormatContext* in_ctx = NULL;
    AVFormatContext* out_ctx = NULL;
    AVCodecContext *video_dec_ctx = NULL, *audio_dec_ctx = NULL;
    AVCodecContext *audio_enc_ctx = NULL;
    AudioProcessingContext ap_ctx = {0};
    AVPacket* pkt = NULL;
    AVFrame* frame = NULL;
    int ret = 0;
    int video_stream_idx = -1;
    int audio_stream_idx = -1;
    int processing_done = 0;
    VTL_AppResult result = VTL_res_ffmpeg_kIOError;

    ret = avformat_open_input(&in_ctx, input_path, NULL, NULL);
    if (ret < 0) {
        char error_buf[256];
        av_strerror(ret, error_buf, sizeof(error_buf));
        av_log(NULL, AV_LOG_ERROR, "Could not open input file '%s': %s\n", input_path, error_buf);
        return VTL_res_video_fs_r_kMissingFileErr;
    }

    ret = avformat_find_stream_info(in_ctx, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not find stream information\n");
        avformat_close_input(&in_ctx);
        return VTL_res_ffmpeg_kFormatError;
    }

    for (int i = 0; i < in_ctx->nb_streams; i++) {
        if (in_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video_stream_idx < 0) {
            video_stream_idx = i;
        } else if (in_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio_stream_idx < 0) {
            audio_stream_idx = i;
        }
    }

    if (video_stream_idx == -1 && audio_stream_idx == -1) {
        av_log(NULL, AV_LOG_ERROR, "Could not find any streams\n");
        avformat_close_input(&in_ctx);
        return VTL_res_ffmpeg_kStreamError;
    }

    ret = avformat_alloc_output_context2(&out_ctx, NULL, NULL, output_path);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not create output context\n");
        avformat_close_input(&in_ctx);
        return VTL_res_ffmpeg_kFormatError;
    }

    if (video_stream_idx != -1) {
        AVStream* in_video_stream = in_ctx->streams[video_stream_idx];
        AVStream* out_video_stream = avformat_new_stream(out_ctx, NULL);
        if (!out_video_stream) {
            av_log(NULL, AV_LOG_ERROR, "Failed to create output video stream\n");
            avformat_free_context(out_ctx);
            avformat_close_input(&in_ctx);
            return VTL_res_ffmpeg_kStreamError;
        }

        ret = avcodec_parameters_copy(out_video_stream->codecpar, in_video_stream->codecpar);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to copy video codec parameters\n");
            avformat_free_context(out_ctx);
            avformat_close_input(&in_ctx);
            return VTL_res_ffmpeg_kCodecError;
        }
        out_video_stream->time_base = in_video_stream->time_base;
    }

    if (audio_stream_idx != -1) {
        const AVCodec* audio_decoder = avcodec_find_decoder(in_ctx->streams[audio_stream_idx]->codecpar->codec_id);
        if (!audio_decoder) {
            av_log(NULL, AV_LOG_ERROR, "Failed to find audio decoder\n");
            avformat_free_context(out_ctx);
            avformat_close_input(&in_ctx);
            return VTL_res_ffmpeg_kCodecError;
        }

        audio_dec_ctx = avcodec_alloc_context3(audio_decoder);
        if (!audio_dec_ctx) {
            av_log(NULL, AV_LOG_ERROR, "Failed to allocate audio decoder context\n");
            avformat_free_context(out_ctx);
            avformat_close_input(&in_ctx);
            return VTL_res_ffmpeg_kCodecError;
        }

        ret = avcodec_parameters_to_context(audio_dec_ctx, in_ctx->streams[audio_stream_idx]->codecpar);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to copy audio decoder parameters\n");
            avcodec_free_context(&audio_dec_ctx);
            avformat_free_context(out_ctx);
            avformat_close_input(&in_ctx);
            return VTL_res_ffmpeg_kCodecError;
        }

        ret = avcodec_open2(audio_dec_ctx, audio_decoder, NULL);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to open audio decoder\n");
            avcodec_free_context(&audio_dec_ctx);
            avformat_free_context(out_ctx);
            avformat_close_input(&in_ctx);
            return VTL_res_ffmpeg_kCodecError;
        }

        ret = init_audio_encoder(&audio_enc_ctx, audio_params);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to initialize audio encoder\n");
            avcodec_free_context(&audio_dec_ctx);
            avformat_free_context(out_ctx);
            avformat_close_input(&in_ctx);
            return VTL_res_ffmpeg_kCodecError;
        }

        AVStream* out_audio_stream = avformat_new_stream(out_ctx, NULL);
        if (!out_audio_stream) {
            av_log(NULL, AV_LOG_ERROR, "Failed to create output audio stream\n");
            avcodec_free_context(&audio_dec_ctx);
            avcodec_free_context(&audio_enc_ctx);
            avformat_free_context(out_ctx);
            avformat_close_input(&in_ctx);
            return VTL_res_ffmpeg_kStreamError;
        }

        ret = avcodec_parameters_from_context(out_audio_stream->codecpar, audio_enc_ctx);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to copy audio encoder parameters\n");
            avcodec_free_context(&audio_dec_ctx);
            avcodec_free_context(&audio_enc_ctx);
            avformat_free_context(out_ctx);
            avformat_close_input(&in_ctx);
            return VTL_res_ffmpeg_kCodecError;
        }
        out_audio_stream->time_base = (AVRational){1, audio_enc_ctx->sample_rate};

        ret = init_audio_processing(&ap_ctx, audio_dec_ctx, audio_enc_ctx);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to initialize audio processing\n");
            avcodec_free_context(&audio_dec_ctx);
            avcodec_free_context(&audio_enc_ctx);
            avformat_free_context(out_ctx);
            avformat_close_input(&in_ctx);
            return VTL_res_ffmpeg_kCodecError;
        }
    }

    if (!(out_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&out_ctx->pb, output_path, AVIO_FLAG_WRITE);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Could not open output file '%s'\n", output_path);
            free_audio_processing(&ap_ctx);
            avcodec_free_context(&audio_dec_ctx);
            avcodec_free_context(&audio_enc_ctx);
            avformat_free_context(out_ctx);
            avformat_close_input(&in_ctx);
            return VTL_res_video_fs_w_kFileIsBusyErr;
        }
    }

    ret = avformat_write_header(out_ctx, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error writing header to output file\n");
        free_audio_processing(&ap_ctx);
        avcodec_free_context(&audio_dec_ctx);
        avcodec_free_context(&audio_enc_ctx);
        if (!(out_ctx->oformat->flags & AVFMT_NOFILE)) avio_closep(&out_ctx->pb);
        avformat_free_context(out_ctx);
        avformat_close_input(&in_ctx);
        return VTL_res_ffmpeg_kIOError;
    }

    frame = av_frame_alloc();
    pkt = av_packet_alloc();
    if (!frame || !pkt) {
        av_log(NULL, AV_LOG_ERROR, "Could not allocate frame or packet\n");
        result = VTL_res_ffmpeg_kIOError;
    } else {
        processing_done = 0;
        while (!processing_done && av_read_frame(in_ctx, pkt) >= 0) {
            AVStream* in_stream = in_ctx->streams[pkt->stream_index];
            AVStream* out_stream = NULL;

            if (pkt->stream_index == video_stream_idx) {
                out_stream = out_ctx->streams[video_stream_idx];
                
                av_packet_rescale_ts(pkt, in_stream->time_base, out_stream->time_base);
                pkt->stream_index = out_stream->index;
                pkt->pos = -1;
                
                ret = av_interleaved_write_frame(out_ctx, pkt);
                if (ret < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Error writing video packet\n");
                    av_packet_unref(pkt);
                    processing_done = 1;
                }
            } else if (pkt->stream_index == audio_stream_idx) {
                out_stream = out_ctx->streams[audio_stream_idx];
                
                ret = avcodec_send_packet(audio_dec_ctx, pkt);
                if (ret) {
                    av_log(NULL, AV_LOG_ERROR, "Error sending audio packet to decoder\n");
                    processing_done = 1;
                }

                int decoding_done = 0;
                while (!decoding_done && ret >= 0) {
                    ret = avcodec_receive_frame(audio_dec_ctx, frame);
                    if (ret == AVERROR(EAGAIN)) {
                        decoding_done = 1;
                    } else if (ret == AVERROR_EOF) {
                        decoding_done = 1;
                    } else if (ret < 0) {
                        av_log(NULL, AV_LOG_ERROR, "Error decoding audio frame\n");
                        processing_done = 1;
                        decoding_done = 1;
                    } else {
                        ret = process_audio_frame(frame, audio_params, &ap_ctx, audio_enc_ctx, out_ctx, out_stream);
                        if (ret) {
                            av_log(NULL, AV_LOG_ERROR, "Error processing audio frame\n");
                            processing_done = 1;
                            decoding_done = 1;
                        }

                        av_frame_unref(frame);
                    }
                }
            } else {
                av_log(NULL, AV_LOG_WARNING, "Skipping unknown stream %d\n", pkt->stream_index);
            }

            av_packet_unref(pkt);
        }

        if (audio_stream_idx != -1 && av_audio_fifo_size(ap_ctx.fifo) > 0 && !processing_done) {
            ret = process_remaining_audio(&ap_ctx, audio_enc_ctx, out_ctx, audio_stream_idx, pkt);
            if (ret < 0) {
                processing_done = 1;
            }
        }

        if (!processing_done) {
            av_write_trailer(out_ctx);
            result = VTL_res_kOk;
        }
    }

    av_frame_free(&frame);
    av_packet_free(&pkt);
    free_audio_processing(&ap_ctx);
    avcodec_free_context(&audio_dec_ctx);
    avcodec_free_context(&audio_enc_ctx);
    avformat_close_input(&in_ctx);
    if (out_ctx && !(out_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&out_ctx->pb);
    }
    avformat_free_context(out_ctx);

    return result;
}