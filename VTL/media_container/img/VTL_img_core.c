#include <VTL/media_container/img/VTL_img_core.h>
#include <VTL/VTL_app_result.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

/**
 * Инициализирует граф фильтров FFmpeg.
 * Добавлена строгая проверка на NULL и безопасная очистка временных структур AVFilterInOut.
 */
static int init_filter_graph(VTL_ImageContext* ctx, const char* filter_descr, AVFrame* input_frame)
{
    if (!ctx || !ctx->current_frame || !filter_descr) return VTL_res_kInvalidParamErr;

    int ret = 0;
    char args[512];
    AVFilterInOut *inputs = NULL;
    AVFilterInOut *outputs = NULL;

    // Выделяем граф
    ctx->filter_graph = avfilter_graph_alloc();
    if (!ctx->filter_graph) return VTL_res_kMemAllocErr;

    // Подготовка параметров входного буфера (формат кадра, размеры, timebase)
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=1/25:pixel_aspect=1/1",
             ctx->current_frame->width, ctx->current_frame->height, ctx->current_frame->format);

    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");

    if (!buffersrc || !buffersink) {
        ret = VTL_res_ffmpeg_kInitError;
        goto cleanup;
    }

    // Создаем входной и выходной фильтры
    if (avfilter_graph_create_filter(&ctx->buffersrc_ctx, buffersrc, "in", args, NULL, ctx->filter_graph) < 0) {
        ret = VTL_res_ffmpeg_kInitError;
        goto cleanup;
    }

    if (avfilter_graph_create_filter(&ctx->buffersink_ctx, buffersink, "out", NULL, NULL, ctx->filter_graph) < 0) {
        ret = VTL_res_ffmpeg_kInitError;
        goto cleanup;
    }

    // Выделяем структуры для парсинга строки фильтров
    inputs = avfilter_inout_alloc();
    outputs = avfilter_inout_alloc();
    if (!inputs || !outputs) {
        ret = VTL_res_kMemAllocErr;
        goto cleanup;
    }

    // Связываем входы/выходы графа
    outputs->name = av_strdup("in");
    outputs->filter_ctx = ctx->buffersrc_ctx;
    outputs->pad_idx = 0;
    outputs->next = NULL;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = ctx->buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = NULL;

    if (!inputs->name || !outputs->name) {
        ret = VTL_res_kMemAllocErr;
        goto cleanup;
    }

    // Парсим строку фильтра (например, "boxblur=5:1")
    if (avfilter_graph_parse_ptr(ctx->filter_graph, filter_descr, &inputs, &outputs, NULL) < 0) {
        ret = VTL_res_ffmpeg_kInitError;
        goto cleanup;
    }

    // Проверяем валидность графа
    if (avfilter_graph_config(ctx->filter_graph, NULL) < 0) {
        ret = VTL_res_ffmpeg_kInitError;
        goto cleanup;
    }

    cleanup:
    // Освобождаем временные структуры (важно: avfilter_inout_free корректно обрабатывает NULL)
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    if (ret < 0 && ctx->filter_graph) {
        avfilter_graph_free(&ctx->filter_graph);
    }
    return ret;
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

    // Закрываем все контексты FFmpeg и освобождаем кадры
    if (ctx->format_ctx) avformat_close_input(&ctx->format_ctx);
    if (ctx->codec_ctx) avcodec_free_context(&ctx->codec_ctx);
    if (ctx->filter_graph) avfilter_graph_free(&ctx->filter_graph);
    if (ctx->sws_ctx) sws_freeContext(ctx->sws_ctx);

    // ДОБАВЛЕНО: Освобождение кадра, чтобы избежать утечек памяти
    if (ctx->current_frame) av_frame_free(&ctx->current_frame);

    free(ctx);
}

VTL_AppResult VTL_img_LoadImage(const char* input_path, VTL_ImageContext* ctx)
{
    if (!input_path || !ctx) return VTL_res_kInvalidParamErr;

    const AVCodec *decoder = NULL;
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    int video_stream_index = -1;
    int ret = VTL_res_kErr;

    if (!packet || !frame) {
        ret = VTL_res_kMemAllocErr;
        goto cleanup;
    }

    // Открываем файл
    if (avformat_open_input(&ctx->format_ctx, input_path, NULL, NULL) < 0) {
        fprintf(stderr, "Failed to open input file: %s\n", input_path);
        ret = VTL_res_ffmpeg_kIOError;
        goto cleanup;
    }

    // Читаем информацию о потоках
    if (avformat_find_stream_info(ctx->format_ctx, NULL) < 0) {
        fprintf(stderr, "Failed to find stream info\n");
        ret = VTL_res_ffmpeg_kFormatError;
        goto cleanup;
    }

    // Ищем видеопоток
    video_stream_index = av_find_best_stream(ctx->format_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
    if (video_stream_index < 0) {
        fprintf(stderr, "Failed to find video stream\n");
        ret = VTL_res_ffmpeg_kStreamError;
        goto cleanup;
    }

    // Инициализируем декодер
    ctx->codec_ctx = avcodec_alloc_context3(decoder);
    if (!ctx->codec_ctx) {
        ret = VTL_res_kMemAllocErr;
        goto cleanup;
    }

    if (avcodec_parameters_to_context(ctx->codec_ctx, ctx->format_ctx->streams[video_stream_index]->codecpar) < 0) {
        ret = VTL_res_ffmpeg_kCodecError;
        goto cleanup;
    }

    if (avcodec_open2(ctx->codec_ctx, decoder, NULL) < 0) {
        ret = VTL_res_ffmpeg_kCodecError;
        goto cleanup;
    }

    // Читаем пакеты до первого видеокадра
    while (av_read_frame(ctx->format_ctx, packet) >= 0) {
        if (packet->stream_index == video_stream_index) {
            if (avcodec_send_packet(ctx->codec_ctx, packet) == 0) {
                if (avcodec_receive_frame(ctx->codec_ctx, frame) == 0) {

                    // ДОБАВЛЕНО: Безопасное освобождение старого кадра при повторной загрузке
                    if (ctx->current_frame) {
                        av_frame_free(&ctx->current_frame);
                    }

                    ctx->current_frame = av_frame_alloc();
                    if (!ctx->current_frame) {
                        ret = VTL_res_kMemAllocErr;
                    } else if (av_frame_ref(ctx->current_frame, frame) < 0) {
                        ret = VTL_res_ffmpeg_kMemoryError;
                        av_frame_free(&ctx->current_frame);
                    } else {
                        ret = VTL_res_kOk;
                    }
                    av_packet_unref(packet);
                    break;
                }
            }
        }
        av_packet_unref(packet);
    }

    cleanup:
    av_frame_free(&frame);
    av_packet_free(&packet);
    return ret;
}

VTL_AppResult VTL_img_SaveImage(const char* output_path, VTL_ImageContext* ctx)
{
    if (!output_path || !ctx || !ctx->current_frame) return VTL_res_kInvalidParamErr;

    AVFormatContext *out_fmt_ctx = NULL;
    AVCodecContext *enc_ctx = NULL;
    AVStream *out_stream = NULL;
    AVPacket *packet = av_packet_alloc();
    AVFrame *rgb_frame = av_frame_alloc();
    struct SwsContext *sws_ctx = NULL;
    int ret = VTL_res_kErr;
    int header_written = 0;

    if (!packet || !rgb_frame) {
        ret = VTL_res_kMemAllocErr;
        goto cleanup;
    }

    // Подготовка RGB кадра (PNG требует RGB или RGBA)
    rgb_frame->format = AV_PIX_FMT_RGB24;
    rgb_frame->width = ctx->current_frame->width;
    rgb_frame->height = ctx->current_frame->height;
    if (av_frame_get_buffer(rgb_frame, 0) < 0) {
        ret = VTL_res_ffmpeg_kMemoryError;
        goto cleanup;
    }

    // Инициализация контекста конвертации (SWS)
    sws_ctx = sws_getContext(
            ctx->current_frame->width, ctx->current_frame->height, ctx->current_frame->format,
            rgb_frame->width, rgb_frame->height, rgb_frame->format,
            SWS_BILINEAR, NULL, NULL, NULL
    );
    if (!sws_ctx) {
        ret = VTL_res_ffmpeg_kConversionError;
        goto cleanup;
    }

    // Выполняем конвертацию цветового пространства
    if (sws_scale(sws_ctx,
                  (const uint8_t * const*)ctx->current_frame->data, ctx->current_frame->linesize,
                  0, ctx->current_frame->height,
                  rgb_frame->data, rgb_frame->linesize) < 0) {
        ret = VTL_res_ffmpeg_kConversionError;
        goto cleanup;
    }

    // Создаем контекст вывода
    if (avformat_alloc_output_context2(&out_fmt_ctx, NULL, NULL, output_path) < 0) {
        ret = VTL_res_ffmpeg_kFormatError;
        goto cleanup;
    }

    // Настраиваем энкодер PNG
    const AVCodec *encoder = avcodec_find_encoder(AV_CODEC_ID_PNG);
    if (!encoder) {
        ret = VTL_res_ffmpeg_kCodecError;
        goto cleanup;
    }

    enc_ctx = avcodec_alloc_context3(encoder);
    if (!enc_ctx) {
        ret = VTL_res_kMemAllocErr;
        goto cleanup;
    }

    enc_ctx->height = rgb_frame->height;
    enc_ctx->width = rgb_frame->width;
    enc_ctx->pix_fmt = rgb_frame->format;
    enc_ctx->time_base = (AVRational){1, 25};

    if (out_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
        enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if (avcodec_open2(enc_ctx, encoder, NULL) < 0) {
        ret = VTL_res_ffmpeg_kCodecError;
        goto cleanup;
    }

    // Добавляем поток в файл
    out_stream = avformat_new_stream(out_fmt_ctx, NULL);
    if (!out_stream) {
        ret = VTL_res_kMemAllocErr;
        goto cleanup;
    }

    if (avcodec_parameters_from_context(out_stream->codecpar, enc_ctx) < 0) {
        ret = VTL_res_ffmpeg_kCodecError;
        goto cleanup;
    }
    out_stream->time_base = enc_ctx->time_base;

    // Открываем файл для записи
    if (avio_open(&out_fmt_ctx->pb, output_path, AVIO_FLAG_WRITE) < 0) {
        fprintf(stderr, "Failed to open output file: %s\n", output_path);
        ret = VTL_res_ffmpeg_kIOError;
        goto cleanup;
    }

    // Записываем заголовок
    if (avformat_write_header(out_fmt_ctx, NULL) < 0) {
        ret = VTL_res_ffmpeg_kFormatError;
        goto cleanup;
    }
    header_written = 1;

    // Кодируем кадр
    if (avcodec_send_frame(enc_ctx, rgb_frame) < 0) {
        ret = VTL_res_ffmpeg_kCodecError;
        goto cleanup;
    }

    while (avcodec_receive_packet(enc_ctx, packet) == 0) {
        packet->stream_index = out_stream->index;
        if (av_interleaved_write_frame(out_fmt_ctx, packet) < 0) {
            ret = VTL_res_ffmpeg_kIOError;
            av_packet_unref(packet);
            goto cleanup;
        }
        av_packet_unref(packet);
    }

    // Завершаем запись
    if (header_written) {
        av_write_trailer(out_fmt_ctx);
    }
    ret = VTL_res_kOk;

    cleanup:
    // ИСПРАВЛЕНО: Гарантированная очистка всех ресурсов при любом исходе
    if (enc_ctx) avcodec_free_context(&enc_ctx);
    if (out_fmt_ctx) {
        if (out_fmt_ctx->pb) avio_closep(&out_fmt_ctx->pb); // Закрытие файлового дескриптора
        avformat_free_context(out_fmt_ctx);
    }
    if (sws_ctx) sws_freeContext(sws_ctx);
    av_frame_free(&rgb_frame);
    av_packet_free(&packet);
    return ret;
}

VTL_AppResult VTL_img_ApplyFilter(VTL_ImageContext* ctx, const VTL_ImageFilter* filter)
{
    if (!ctx || !filter || !ctx->current_frame) return VTL_res_kInvalidParamErr;

    AVFrame *filt_frame = av_frame_alloc();
    if (!filt_frame) return VTL_res_kMemAllocErr;

    int ret = VTL_res_kOk;

    // Инициализация графа фильтров
    if (init_filter_graph(ctx, filter->filter_desc, ctx->current_frame) < 0) {
        ret = VTL_res_ffmpeg_kInitError;
        goto cleanup;
    }

    // Отправляем кадр в фильтр
    if (av_buffersrc_add_frame(ctx->buffersrc_ctx, ctx->current_frame) < 0) {
        ret = VTL_res_ffmpeg_kIOError;
        goto cleanup;
    }

    // Получаем отфильтрованный кадр
    if (av_buffersink_get_frame(ctx->buffersink_ctx, filt_frame) < 0) {
        ret = VTL_res_ffmpeg_kIOError;
        goto cleanup;
    }

    // ДОБАВЛЕНО: Безопасное обновление текущего кадра через ссылки FFmpeg
    av_frame_unref(ctx->current_frame);
    if (av_frame_ref(ctx->current_frame, filt_frame) < 0) {
        ret = VTL_res_ffmpeg_kMemoryError;
        goto cleanup;
    }

    cleanup:
    av_frame_free(&filt_frame);
    return ret;
}

VTL_AppResult VTL_img_InitGPU(void)
{
    AVBufferRef* hw_device_ctx = NULL;
    // Пробуем CUDA, затем OpenCL
    int err = av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_CUDA, NULL, NULL, 0);
    if (err < 0) {
        err = av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_OPENCL, NULL, NULL, 0);
        if (err < 0) {
            fprintf(stderr, "Failed to create hardware device context\n");
            return VTL_res_ffmpeg_kInitError;
        }
    }

    av_buffer_unref(&hw_device_ctx);
    return VTL_res_kOk;
}

void VTL_img_CleanupGPU(void)
{
    // Очистка GPU ресурсов (в данной реализации автоматическая через граф фильтров)
}

/**
 * Структура для передачи данных в поток обработки изображения.
 */
typedef struct {
    const char* input_path;
    const VTL_ImageFilter* filter;
    const char* output_path;
    VTL_AppResult result;
} VTL_BatchTask;

/**
 * Воркер-функция для потока. Выполняет полный цикл обработки одного изображения.
 */
static void* vtl_img_worker(void* arg)
{
    VTL_BatchTask* task = (VTL_BatchTask*)arg;

    // Каждому потоку нужен свой независимый контекст FFmpeg
    VTL_ImageContext* ctx = VTL_img_context_Init();
    if (!ctx) {
        task->result = VTL_res_kMemAllocErr;
        return NULL;
    }

    // Загрузка
    task->result = VTL_img_LoadImage(task->input_path, ctx);

    // Фильтрация (если задана)
    if (task->result == VTL_res_kOk && task->filter) {
        task->result = VTL_img_ApplyFilter(ctx, task->filter);
    }

    // Сохранение
    if (task->result == VTL_res_kOk) {
        task->result = VTL_img_SaveImage(task->output_path, ctx);
    }

    VTL_img_context_Cleanup(ctx);
    return NULL;
}

VTL_AppResult VTL_img_ProcessBatch(const char** input_paths, const VTL_ImageFilter** filters, const char** output_paths, int count)
{
    if (!input_paths || !output_paths || count <= 0) return VTL_res_kInvalidParamErr;

    // Создаем массив задач и потоков
    VTL_BatchTask* tasks = (VTL_BatchTask*)malloc(sizeof(VTL_BatchTask) * count);
    pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t) * count);

    if (!tasks || !threads) {
        free(tasks);
        free(threads);
        return VTL_res_kMemAllocErr;
    }

    // Запускаем потоки
    for (int i = 0; i < count; i++) {
        tasks[i].input_path = input_paths[i];
        tasks[i].filter = (filters) ? filters[i] : NULL;
        tasks[i].output_path = output_paths[i];
        tasks[i].result = VTL_res_kOk;

        if (pthread_create(&threads[i], NULL, vtl_img_worker, &tasks[i]) != 0) {
            fprintf(stderr, "Failed to create thread for image %d\n", i);
            tasks[i].result = VTL_res_kErr;
        }
    }

    // Ожидаем завершения всех потоков
    for (int i = 0; i < count; i++) {
        pthread_join(threads[i], NULL);
        if (tasks[i].result != VTL_res_kOk) {
            printf("Image failed with fail %d\n", i);
        }
    }

    free(tasks);
    free(threads);
    return VTL_res_kOk;
}
