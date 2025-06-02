#include "VTL_video_converter.h"
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <stdlib.h>

VTL_AppResult VTL_convert_video_format(const char* input_path, const char* output_path) {
    AVFormatContext *input_ctx = NULL, *output_ctx = NULL;
    AVPacket packet;
    int ret = 0;
    int processing_complete = 0;

    // 1. Открываем входной файл
    if ((ret = avformat_open_input(&input_ctx, input_path, NULL, NULL))) {
        return VTL_res_video_fs_r_kMissingFileErr;
    }

    if (!processing_complete) {
        if ((ret = avformat_find_stream_info(input_ctx, NULL))) {
            avformat_close_input(&input_ctx);
            return VTL_res_ffmpeg_kFormatError;
        }

        // 2. Создаем выходной контекст MP4
        avformat_alloc_output_context2(&output_ctx, NULL, "mp4", output_path);
        if (!output_ctx) {
            avformat_close_input(&input_ctx);
            return VTL_res_ffmpeg_kFormatError;
        }

        // 3. Копируем потоки
        int stream_copy_success = 1;
        for (int i = 0; i < input_ctx->nb_streams && stream_copy_success; i++) {
            AVStream *in_stream = input_ctx->streams[i];
            AVStream *out_stream = avformat_new_stream(output_ctx, NULL);
            if (!out_stream) {
                stream_copy_success = 0;
                ret = VTL_res_ffmpeg_kMemoryError;
            } else {
                ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
                if (ret) {
                    stream_copy_success = 0;
                    ret = VTL_res_ffmpeg_kStreamError;
                } else if (out_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                    out_stream->codecpar->codec_id = AV_CODEC_ID_AAC;
                }
            }
        }

        if (!stream_copy_success) {
            avformat_close_input(&input_ctx);
            avformat_free_context(output_ctx);
            return ret;
        }

        // 4. Открываем выходной файл
        if (!(output_ctx->oformat->flags & AVFMT_NOFILE)) {
            if ((ret = avio_open(&output_ctx->pb, output_path, AVIO_FLAG_WRITE))) {
                avformat_close_input(&input_ctx);
                avformat_free_context(output_ctx);
                return VTL_res_video_fs_w_kFileIsBusyErr;
            }
        }

        // 5. Записываем заголовок
        if ((ret = avformat_write_header(output_ctx, NULL))) {
            avformat_close_input(&input_ctx);
            if (!(output_ctx->oformat->flags & AVFMT_NOFILE)) {
                avio_closep(&output_ctx->pb);
            }
            avformat_free_context(output_ctx);
            return VTL_res_ffmpeg_kIOError;
        }

        // 6. Обработка пакетов
        int read_success = 1;
        while (read_success && !processing_complete) {
            read_success = av_read_frame(input_ctx, &packet) >= 0;
            if (read_success) {
                AVStream *in_stream = input_ctx->streams[packet.stream_index];
                AVStream *out_stream = output_ctx->streams[packet.stream_index];

                packet.pts = av_rescale_q(packet.pts, in_stream->time_base, out_stream->time_base);
                packet.dts = av_rescale_q(packet.dts, in_stream->time_base, out_stream->time_base);
                packet.duration = av_rescale_q(packet.duration, in_stream->time_base, out_stream->time_base);
                packet.pos = -1;

                if ((ret = av_interleaved_write_frame(output_ctx, &packet))) {
                    processing_complete = 1;
                }
                av_packet_unref(&packet);
            }
        }

        // 7. Завершение записи
        if (!processing_complete) {
            av_write_trailer(output_ctx);
        }
    }

    // 8. Очистка
    avformat_close_input(&input_ctx);
    if (output_ctx && !(output_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&output_ctx->pb);
    }
    avformat_free_context(output_ctx);

    return (ret >= 0) ? VTL_res_kOk : VTL_res_ffmpeg_kConversionError;
}