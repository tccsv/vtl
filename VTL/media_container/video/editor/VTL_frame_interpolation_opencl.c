#include "VTL_frame_interpolation.h"
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_opencl.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <CL/opencl.h>

#define LOG_DEBUG(msg, ...) av_log(NULL, AV_LOG_DEBUG, "[DEBUG] %s:%d: " msg, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_ERROR(msg, ...) av_log(NULL, AV_LOG_ERROR, "[ERROR] %s:%d: " msg, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_WARNING(msg, ...) av_log(NULL, AV_LOG_WARNING, "[WARN] %s:%d: " msg, __FILE__, __LINE__, ##__VA_ARGS__)

// OpenCL kernel source code
static const char *interpolation_kernel_source =
    "__kernel void interpolate_frames(__global const uchar *prev_frame,   \n"
    "                                __global const uchar *next_frame,    \n"
    "                                __global uchar *out_frame,           \n"
    "                                const int width,                     \n"
    "                                const int height,                    \n"
    "                                const float factor,                  \n"
    "                                const int prev_pitch,                \n"
    "                                const int next_pitch,                \n"
    "                                const int out_pitch) {               \n"
    "    int x = get_global_id(0);                                        \n"
    "    int y = get_global_id(1);                                        \n"
    "                                                                     \n"
    "    if (x >= width || y >= height) return;                           \n"
    "                                                                     \n"
    "    int prev_idx = y * prev_pitch + x;                               \n"
    "    int next_idx = y * next_pitch + x;                               \n"
    "    int out_idx = y * out_pitch + x;                                 \n"
    "                                                                     \n"
    "    out_frame[out_idx] = (uchar)(prev_frame[prev_idx] * (1.0f - factor) + \n"
    "                                 next_frame[next_idx] * factor);      \n"
    "}                                                                   \n";

typedef struct
{
    AVFormatContext *fmt_ctx;
    AVCodecContext *dec_ctx;
    AVCodecContext *enc_ctx;
    AVBufferRef *hw_device_ref;
    int video_stream_index;

    // OpenCL specific members
    cl_context cl_ctx;
    cl_command_queue cl_queue;
    cl_program cl_program;
    cl_kernel cl_kernel;
    cl_mem cl_prev_frame;
    cl_mem cl_next_frame;
    cl_mem cl_out_frame;

    VTL_InterpolationParams params;

    AVFrame *prev_frame;
    AVFrame *next_frame;
    AVFrame *interpolated_frame;

    AVFilterGraph *filter_graph;
    AVFilterContext *src_ctx;
    AVFilterContext *sink_ctx;

    AVFormatContext *out_ctx;
    AVStream *out_stream;
} VTL_Interpolator;

/* Вспомогательные статусы для обработки кодировщика */
typedef enum {
    VTL_ENCODER_OK,
    VTL_ENCODER_NEED_MORE,
    VTL_ENCODER_FLUSHED,
    VTL_ENCODER_ERROR
} VTL_EncoderStatus;

static void VTL_log_opencl_device_info(cl_device_id device)
{
    char device_name[128];
    char device_vendor[128];
    char device_version[128];

    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    clGetDeviceInfo(device, CL_DEVICE_VENDOR, sizeof(device_vendor), device_vendor, NULL);
    clGetDeviceInfo(device, CL_DEVICE_VERSION, sizeof(device_version), device_version, NULL);

    LOG_DEBUG("OpenCL Device Info:\n");
    LOG_DEBUG("  Name: %s\n", device_name);
    LOG_DEBUG("  Vendor: %s\n", device_vendor);
    LOG_DEBUG("  Version: %s\n", device_version);
}

static VTL_AppResult VTL_init_opencl_platform_device(cl_platform_id* platform, cl_device_id* device) {
    cl_int err = clGetPlatformIDs(1, platform, NULL);
    if (err != CL_SUCCESS) {
        LOG_ERROR("Failed to get OpenCL platform: %d\n", err);
        return VTL_res_ffmpeg_kInitError;
    }

    err = clGetDeviceIDs(*platform, CL_DEVICE_TYPE_GPU, 1, device, NULL);
    if (err != CL_SUCCESS) {
        LOG_ERROR("Failed to get OpenCL GPU device: %d\n", err);
        return VTL_res_ffmpeg_kInitError;
    }

    return VTL_res_kOk;
}

static VTL_AppResult VTL_create_opencl_context_queue(VTL_Interpolator* interp, cl_device_id device) {
    cl_int err;
    
    interp->cl_ctx = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (!interp->cl_ctx || err != CL_SUCCESS) {
        LOG_ERROR("Failed to create OpenCL context: %d\n", err);
        return VTL_res_ffmpeg_kInitError;
    }

    interp->cl_queue = clCreateCommandQueue(interp->cl_ctx, device, 0, &err);
    if (!interp->cl_queue || err != CL_SUCCESS) {
        LOG_ERROR("Failed to create OpenCL command queue: %d\n", err);
        return VTL_res_ffmpeg_kInitError;
    }

    return VTL_res_kOk;
}

static VTL_AppResult VTL_create_opencl_program(VTL_Interpolator* interp, cl_device_id device) {
    cl_int err;
    
    interp->cl_program = clCreateProgramWithSource(interp->cl_ctx, 1,
                                                 &interpolation_kernel_source, NULL, &err);
    if (!interp->cl_program || err != CL_SUCCESS) {
        LOG_ERROR("Failed to create OpenCL program: %d\n", err);
        return VTL_res_ffmpeg_kInitError;
    }

    err = clBuildProgram(interp->cl_program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        char build_log[4096];
        clGetProgramBuildInfo(interp->cl_program, device, CL_PROGRAM_BUILD_LOG,
                            sizeof(build_log), build_log, NULL);
        LOG_ERROR("Failed to build OpenCL program: %d\nLog:\n%s\n", err, build_log);
        return VTL_res_ffmpeg_kInitError;
    }

    return VTL_res_kOk;
}

static VTL_AppResult VTL_create_opencl_kernel(VTL_Interpolator* interp) {
    cl_int err;
    
    interp->cl_kernel = clCreateKernel(interp->cl_program, "interpolate_frames", &err);
    if (!interp->cl_kernel || err != CL_SUCCESS) {
        LOG_ERROR("Failed to create OpenCL kernel: %d\n", err);
        return VTL_res_ffmpeg_kInitError;
    }

    return VTL_res_kOk;
}

static VTL_AppResult VTL_create_opencl_buffers(VTL_Interpolator* interp) {
    cl_int err;
    size_t y_size = interp->params.width * interp->params.height;
    size_t uv_size = (interp->params.width / 2) * (interp->params.height / 2);
    size_t total_size = y_size + 2 * uv_size;

    interp->cl_prev_frame = clCreateBuffer(interp->cl_ctx, CL_MEM_READ_ONLY,
                                         total_size, NULL, &err);
    interp->cl_next_frame = clCreateBuffer(interp->cl_ctx, CL_MEM_READ_ONLY,
                                         total_size, NULL, &err);
    interp->cl_out_frame = clCreateBuffer(interp->cl_ctx, CL_MEM_WRITE_ONLY,
                                        total_size, NULL, &err);

    if (!interp->cl_prev_frame || !interp->cl_next_frame || 
        !interp->cl_out_frame || err != CL_SUCCESS) {
        LOG_ERROR("Failed to create OpenCL buffers: %d\n", err);
        return VTL_res_ffmpeg_kInitError;
    }

    return VTL_res_kOk;
}

static VTL_AppResult VTL_init_opencl(VTL_Interpolator* interp) {
    cl_platform_id platform;
    cl_device_id device;
    VTL_AppResult res;

    if ((res = VTL_init_opencl_platform_device(&platform, &device)) != VTL_res_kOk) {
        return res;
    }

    VTL_log_opencl_device_info(device);

    if ((res = VTL_create_opencl_context_queue(interp, device)) != VTL_res_kOk) {
        return res;
    }

    if ((res = VTL_create_opencl_program(interp, device)) != VTL_res_kOk) {
        return res;
    }

    if ((res = VTL_create_opencl_kernel(interp)) != VTL_res_kOk) {
        return res;
    }

    return VTL_create_opencl_buffers(interp);
}

static VTL_AppResult VTL_cleanup_opencl(VTL_Interpolator *interp)
{
    if (interp->cl_prev_frame)
        clReleaseMemObject(interp->cl_prev_frame);
    if (interp->cl_next_frame)
        clReleaseMemObject(interp->cl_next_frame);
    if (interp->cl_out_frame)
        clReleaseMemObject(interp->cl_out_frame);
    if (interp->cl_kernel)
        clReleaseKernel(interp->cl_kernel);
    if (interp->cl_program)
        clReleaseProgram(interp->cl_program);
    if (interp->cl_queue)
        clReleaseCommandQueue(interp->cl_queue);
    if (interp->cl_ctx)
        clReleaseContext(interp->cl_ctx);

    return VTL_res_kOk;
}

static VTL_AppResult VTL_open_input_file(VTL_Interpolator *interp, const char *filename) {
    if (!interp || !filename) {
        LOG_ERROR("Invalid parameters\n");
        return VTL_res_ffmpeg_kInitError;
    }

    LOG_DEBUG("Opening input file: %s\n", filename);
    int ret = avformat_open_input(&interp->fmt_ctx, filename, NULL, NULL);
    if (ret < 0) {
        LOG_ERROR("Cannot open input file '%s': %s\n", filename, av_err2str(ret));
        return VTL_res_video_fs_r_kMissingFileErr;
    }

    LOG_DEBUG("Finding stream info\n");
    ret = avformat_find_stream_info(interp->fmt_ctx, NULL);
    if (ret < 0) {
        LOG_ERROR("Cannot find stream info: %s\n", av_err2str(ret));
        return VTL_res_ffmpeg_kStreamError;
    }

    return VTL_res_kOk;
}

static VTL_AppResult VTL_init_video_decoder(VTL_Interpolator *interp) {
    AVCodec *decoder = NULL;
    LOG_DEBUG("Finding video stream\n");
    int ret = av_find_best_stream(interp->fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
    if (ret < 0) {
        LOG_ERROR("Cannot find video stream: %s\n", av_err2str(ret));
        return VTL_res_ffmpeg_kStreamError;
    }
    
    interp->video_stream_index = ret;
    LOG_DEBUG("Found video stream at index %d\n", interp->video_stream_index);

    interp->dec_ctx = avcodec_alloc_context3(decoder);
    if (!interp->dec_ctx) {
        LOG_ERROR("Cannot allocate decoder context\n");
        return VTL_res_ffmpeg_kInitError;
    }

    LOG_DEBUG("Copying stream parameters to decoder\n");
    ret = avcodec_parameters_to_context(interp->dec_ctx,
                                      interp->fmt_ctx->streams[interp->video_stream_index]->codecpar);
    if (ret < 0) {
        LOG_ERROR("Cannot copy stream parameters: %s\n", av_err2str(ret));
        return VTL_res_ffmpeg_kStreamError;
    }

    LOG_DEBUG("Opening decoder for codec: %s\n", avcodec_get_name(interp->dec_ctx->codec_id));
    ret = avcodec_open2(interp->dec_ctx, decoder, NULL);
    if (ret < 0) {
        LOG_ERROR("Cannot open decoder: %s\n", av_err2str(ret));
        return VTL_res_ffmpeg_kCodecError;
    }

    return VTL_res_kOk;
}

static VTL_AppResult VTL_init_frame_buffers(VTL_Interpolator *interp) {
    LOG_DEBUG("Allocating frame buffers\n");
    interp->prev_frame = av_frame_alloc();
    interp->next_frame = av_frame_alloc();
    interp->interpolated_frame = av_frame_alloc();

    if (!interp->prev_frame || !interp->next_frame || !interp->interpolated_frame) {
        LOG_ERROR("Cannot allocate frames\n");
        return VTL_res_ffmpeg_kInitError;
    }

    AVFrame *frames[] = {interp->prev_frame, interp->next_frame, interp->interpolated_frame};
    for (int i = 0; i < 3; i++) {
        frames[i]->width = interp->dec_ctx->width;
        frames[i]->height = interp->dec_ctx->height;
        frames[i]->format = AV_PIX_FMT_YUV420P;
        if (av_frame_get_buffer(frames[i], 32)) {
            LOG_ERROR("Cannot allocate frame buffers\n");
            return VTL_res_ffmpeg_kInitError;
        }
    }

    return VTL_res_kOk;
}

static VTL_AppResult VTL_init_video_context(VTL_Interpolator *interp, const char *filename) {
    VTL_AppResult res;
    
    if ((res = VTL_open_input_file(interp, filename))) {
        return res;
    }
    
    if ((res = VTL_init_video_decoder(interp))) {
        return res;
    }
    
    return VTL_init_frame_buffers(interp);
}

static VTL_AppResult VTL_convert_frame_to_yuv420p(AVFrame *src, AVFrame **dst)
{
    if (!src || !dst)
    {
        LOG_ERROR("Invalid parameters\n");
        return VTL_res_ffmpeg_kInitError;
    }

    *dst = av_frame_alloc();
    if (!*dst)
    {
        LOG_ERROR("Cannot allocate frame\n");
        return VTL_res_ffmpeg_kInitError;
    }

    (*dst)->width = src->width;
    (*dst)->height = src->height;
    (*dst)->format = AV_PIX_FMT_YUV420P;
    (*dst)->pts = src->pts;

    if (av_frame_get_buffer(*dst, 32))
    {
        LOG_ERROR("Cannot allocate frame buffer\n");
        av_frame_free(dst);
        return VTL_res_ffmpeg_kInitError;
    }

    struct SwsContext *sws_ctx = sws_getContext(
        src->width, src->height, src->format,
        src->width, src->height, AV_PIX_FMT_YUV420P,
        SWS_BILINEAR, NULL, NULL, NULL);

    if (!sws_ctx)
    {
        LOG_ERROR("Cannot create scaling context\n");
        av_frame_free(dst);
        return VTL_res_ffmpeg_kInitError;
    }

    sws_scale(sws_ctx,
              (const uint8_t *const *)src->data, src->linesize,
              0, src->height,
              (*dst)->data, (*dst)->linesize);

    sws_freeContext(sws_ctx);

    return VTL_res_kOk;
}

static VTL_AppResult VTL_init_output_context(VTL_Interpolator *interp, const char *filename)
{
    if (!interp || !filename)
    {
        LOG_ERROR("Invalid parameters\n");
        return VTL_res_ffmpeg_kInitError;
    }

    LOG_DEBUG("Initializing output context for file: %s\n", filename);

    VTL_AppResult result = VTL_res_kOk;
    
    result = VTL_find_and_setup_encoder(interp);
    if (result != VTL_res_kOk) return result;
    
    result = VTL_create_output_context(interp, filename);
    if (result != VTL_res_kOk) return result;
    
    result = VTL_configure_encoder(interp);
    if (result != VTL_res_kOk) return result;
    
    result = VTL_open_output_file(interp, filename);
    if (result != VTL_res_kOk) return result;
    
    LOG_DEBUG("Output context initialized successfully\n");
    return VTL_res_kOk;
}

static VTL_AppResult VTL_find_and_setup_encoder(VTL_Interpolator *interp)
{
    AVCodec *encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!encoder)
    {
        encoder = avcodec_find_encoder(interp->dec_ctx->codec_id);
        if (!encoder)
        {
            LOG_ERROR("Cannot find encoder for codec %s\n",
                      avcodec_get_name(interp->dec_ctx->codec_id));
            return VTL_res_ffmpeg_kCodecError;
        }
    }
    LOG_DEBUG("Using encoder: %s\n", encoder->name);
    
    interp->enc_ctx = avcodec_alloc_context3(encoder);
    if (!interp->enc_ctx)
    {
        LOG_ERROR("Cannot allocate encoder context\n");
        return VTL_res_ffmpeg_kInitError;
    }
    
    return VTL_res_kOk;
}

static VTL_AppResult VTL_create_output_context(VTL_Interpolator *interp, const char *filename)
{
    int ret = avformat_alloc_output_context2(&interp->out_ctx, NULL, NULL, filename);
    if (ret < 0)
    {
        LOG_ERROR("Cannot create output context: %s\n", av_err2str(ret));
        return VTL_res_ffmpeg_kFormatError;
    }

    interp->out_stream = avformat_new_stream(interp->out_ctx, interp->enc_ctx->codec);
    if (!interp->out_stream)
    {
        LOG_ERROR("Cannot create output stream\n");
        return VTL_res_ffmpeg_kInitError;
    }
    
    return VTL_res_kOk;
}

static VTL_AppResult VTL_configure_encoder(VTL_Interpolator *interp)
{
    interp->enc_ctx->width = interp->params.width;
    interp->enc_ctx->height = interp->params.height;
    interp->enc_ctx->sample_aspect_ratio = (AVRational){1, 1};
    interp->enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    interp->enc_ctx->time_base = (AVRational){1, interp->params.target_frame_rate};
    interp->enc_ctx->framerate = (AVRational){interp->params.target_frame_rate, 1};
    interp->enc_ctx->bit_rate = 8000000;
    interp->enc_ctx->gop_size = 10;
    interp->enc_ctx->max_b_frames = 1;

    if (interp->out_ctx->oformat->flags & AVFMT_GLOBALHEADER)
    {
        interp->enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    AVDictionary *opts = NULL;
    av_dict_set(&opts, "preset", "fast", 0);
    av_dict_set(&opts, "crf", "23", 0);

    int ret = avcodec_open2(interp->enc_ctx, interp->enc_ctx->codec, &opts);
    av_dict_free(&opts);
    if (ret < 0)
    {
        LOG_ERROR("Cannot open encoder: %s\n", av_err2str(ret));
        return VTL_res_ffmpeg_kCodecError;
    }

    ret = avcodec_parameters_from_context(interp->out_stream->codecpar, interp->enc_ctx);
    if (ret < 0)
    {
        LOG_ERROR("Cannot copy encoder parameters: %s\n", av_err2str(ret));
        return VTL_res_ffmpeg_kStreamError;
    }

    interp->out_stream->time_base = interp->enc_ctx->time_base;
    
    return VTL_res_kOk;
}

static VTL_AppResult VTL_open_output_file(VTL_Interpolator *interp, const char *filename)
{
    if (!(interp->out_ctx->oformat->flags & AVFMT_NOFILE))
    {
        LOG_DEBUG("Opening output file\n");
        int ret = avio_open(&interp->out_ctx->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0)
        {
            LOG_ERROR("Cannot open output file: %s\n", av_err2str(ret));
            return VTL_res_video_fs_w_kFileIsBusyErr;
        }
    }

    LOG_DEBUG("Writing file header\n");
    int ret = avformat_write_header(interp->out_ctx, NULL);
    if (ret < 0)
    {
        LOG_ERROR("Cannot write header: %s\n", av_err2str(ret));
        return VTL_res_ffmpeg_kIOError;
    }
    
    return VTL_res_kOk;
}

static VTL_AppResult VTL_encode_and_write_frame(VTL_Interpolator *interp, AVFrame *frame, int flush)
{
    if (!interp) {
        LOG_ERROR("Invalid interpolator pointer\n");
        return VTL_res_ffmpeg_kInitError;
    }

    AVFrame *yuv_frame = NULL;
    VTL_AppResult res = VTL_handle_frame_conversion(interp, frame, flush, &yuv_frame);
    if (res != VTL_res_kOk) {
        return res;
    }

    res = VTL_handle_encoding(interp, yuv_frame, flush);
    av_frame_free(&yuv_frame);
    
    return res;
}

static VTL_AppResult VTL_handle_frame_conversion(VTL_Interpolator *interp, 
                                               AVFrame *frame, 
                                               int flush,
                                               AVFrame **out_frame)
{
    if (flush || !frame) {
        return VTL_res_kOk;
    }

    LOG_DEBUG("Converting frame to YUV420P, pts=%" PRId64 "\n", frame->pts);
    
    VTL_AppResult res = VTL_convert_frame_to_yuv420p(frame, out_frame);
    if (res != VTL_res_kOk) {
        LOG_ERROR("Frame conversion failed\n");
        return res;
    }
    
    (*out_frame)->pts = frame->pts;
    return VTL_res_kOk;
}

static VTL_AppResult VTL_handle_encoding(VTL_Interpolator *interp, 
                                       AVFrame *frame, 
                                       int flush)
{
    LOG_DEBUG("%s frame (pts=%" PRId64 ")\n", 
             flush ? "Flushing" : "Encoding", 
             frame ? frame->pts : -1);

    int ret = avcodec_send_frame(interp->enc_ctx, flush ? NULL : frame);
    if (ret < 0) {
        LOG_ERROR("Error sending %s to encoder: %s\n",
                flush ? "flush" : "frame", 
                av_err2str(ret));
        return VTL_res_ffmpeg_kCodecError;
    }

    return VTL_process_encoder_output(interp);
}

static VTL_AppResult VTL_process_encoder_output(VTL_Interpolator *interp)
{
    static int64_t last_dts = AV_NOPTS_VALUE;
    VTL_EncoderStatus enc_status;
    int processing_complete = 0;

    while (!processing_complete) {
        AVPacket pkt = {0};
        av_init_packet(&pkt);

        enc_status = VTL_receive_encoder_packet(interp, &pkt);
        if (enc_status == VTL_ENCODER_NEED_MORE) {
            av_packet_unref(&pkt);
            processing_complete = 1;
        } else if (enc_status == VTL_ENCODER_FLUSHED) {
            av_packet_unref(&pkt);
            processing_complete = 1;
        } else if (enc_status == VTL_ENCODER_ERROR) {
            av_packet_unref(&pkt);
            return VTL_res_ffmpeg_kCodecError;
        } else {
            VTL_AppResult res = VTL_handle_output_packet(interp, &pkt, &last_dts);
            av_packet_unref(&pkt);
            if (res != VTL_res_kOk) {
                return res;
            }
        }
    }

    return VTL_res_kOk;
}

static VTL_EncoderStatus VTL_receive_encoder_packet(VTL_Interpolator *interp,
                                                  AVPacket *pkt)
{
    int ret = avcodec_receive_packet(interp->enc_ctx, pkt);
    
    if (ret == AVERROR(EAGAIN)) {
        LOG_DEBUG("Encoder needs more frames\n");
        return VTL_ENCODER_NEED_MORE;
    }
    if (ret == AVERROR_EOF) {
        LOG_DEBUG("Encoder flush complete\n");
        return VTL_ENCODER_FLUSHED;
    }
    if (ret < 0) {
        LOG_ERROR("Error receiving packet: %s\n", av_err2str(ret));
        return VTL_ENCODER_ERROR;
    }

    LOG_DEBUG("Encoded packet: pts=%" PRId64 ", dts=%" PRId64 ", size=%d\n",
             pkt->pts, pkt->dts, pkt->size);
    return VTL_ENCODER_OK;
}

static VTL_AppResult VTL_handle_output_packet(VTL_Interpolator *interp,
                                           AVPacket *pkt,
                                           int64_t *last_dts)
{
    if (pkt->pts != AV_NOPTS_VALUE) {
        pkt->pts = av_rescale_q(pkt->pts, interp->enc_ctx->time_base, 
                               interp->out_stream->time_base);
    }
    if (pkt->dts != AV_NOPTS_VALUE) {
        pkt->dts = av_rescale_q(pkt->dts, interp->enc_ctx->time_base,
                               interp->out_stream->time_base);
    }

    if (*last_dts != AV_NOPTS_VALUE && pkt->dts <= *last_dts) {
        pkt->dts = *last_dts + 1;
        if (pkt->pts < pkt->dts) {
            pkt->pts = pkt->dts;
        }
    }
    *last_dts = pkt->dts;

    pkt->stream_index = interp->out_stream->index;
    int ret = av_interleaved_write_frame(interp->out_ctx, pkt);
    if (ret < 0) {
        LOG_ERROR("Error writing packet: %s\n", av_err2str(ret));
        return VTL_res_ffmpeg_kIOError;
    }

    return VTL_res_kOk;
}

static VTL_AppResult VTL_upload_frame_to_gpu(VTL_Interpolator *interp, AVFrame *frame, cl_mem cl_buffer)
{
    cl_int err;
    size_t y_size = interp->params.width * interp->params.height;
    size_t uv_size = (interp->params.width / 2) * (interp->params.height / 2);

    err = clEnqueueWriteBuffer(interp->cl_queue, cl_buffer, CL_TRUE, 0,
                               y_size, frame->data[0], 0, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        LOG_ERROR("Failed to upload Y plane to GPU: %d\n", err);
        return VTL_res_ffmpeg_kInitError;
    }

    err = clEnqueueWriteBuffer(interp->cl_queue, cl_buffer, CL_TRUE, y_size,
                               uv_size, frame->data[1], 0, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        LOG_ERROR("Failed to upload U plane to GPU: %d\n", err);
        return VTL_res_ffmpeg_kInitError;
    }

    err = clEnqueueWriteBuffer(interp->cl_queue, cl_buffer, CL_TRUE, y_size + uv_size,
                               uv_size, frame->data[2], 0, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        LOG_ERROR("Failed to upload V plane to GPU: %d\n", err);
        return VTL_res_ffmpeg_kInitError;
    }

    return VTL_res_kOk;
}

static VTL_AppResult VTL_download_frame_from_gpu(VTL_Interpolator *interp, cl_mem cl_buffer, AVFrame *frame)
{
    cl_int err;
    size_t y_size = interp->params.width * interp->params.height;
    size_t uv_size = (interp->params.width / 2) * (interp->params.height / 2);

    err = clEnqueueReadBuffer(interp->cl_queue, cl_buffer, CL_TRUE, 0,
                              y_size, frame->data[0], 0, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        LOG_ERROR("Failed to download Y plane from GPU: %d\n", err);
        return VTL_res_ffmpeg_kInitError;
    }

    err = clEnqueueReadBuffer(interp->cl_queue, cl_buffer, CL_TRUE, y_size,
                              uv_size, frame->data[1], 0, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        LOG_ERROR("Failed to download U plane from GPU: %d\n", err);
        return VTL_res_ffmpeg_kInitError;
    }

    err = clEnqueueReadBuffer(interp->cl_queue, cl_buffer, CL_TRUE, y_size + uv_size,
                              uv_size, frame->data[2], 0, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        LOG_ERROR("Failed to download V plane from GPU: %d\n", err);
        return VTL_res_ffmpeg_kInitError;
    }

    return VTL_res_kOk;
}

static VTL_AppResult VTL_run_interpolation_kernel(VTL_Interpolator *interp, float factor, int plane,
                                                  int width, int height, int prev_pitch, int next_pitch, int out_pitch)
{
    cl_int err;

    err = clSetKernelArg(interp->cl_kernel, 0, sizeof(cl_mem), &interp->cl_prev_frame);
    err |= clSetKernelArg(interp->cl_kernel, 1, sizeof(cl_mem), &interp->cl_next_frame);
    err |= clSetKernelArg(interp->cl_kernel, 2, sizeof(cl_mem), &interp->cl_out_frame);
    err |= clSetKernelArg(interp->cl_kernel, 3, sizeof(int), &width);
    err |= clSetKernelArg(interp->cl_kernel, 4, sizeof(int), &height);
    err |= clSetKernelArg(interp->cl_kernel, 5, sizeof(float), &factor);
    err |= clSetKernelArg(interp->cl_kernel, 6, sizeof(int), &prev_pitch);
    err |= clSetKernelArg(interp->cl_kernel, 7, sizeof(int), &next_pitch);
    err |= clSetKernelArg(interp->cl_kernel, 8, sizeof(int), &out_pitch);

    if (err != CL_SUCCESS)
    {
        LOG_ERROR("Failed to set kernel arguments: %d\n", err);
        return VTL_res_ffmpeg_kInitError;
    }

    size_t global_work_size[2] = {width, height};
    err = clEnqueueNDRangeKernel(interp->cl_queue, interp->cl_kernel, 2, NULL,
                                 global_work_size, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        LOG_ERROR("Failed to execute kernel: %d\n", err);
        return VTL_res_ffmpeg_kInitError;
    }

    return VTL_res_kOk;
}

static VTL_AppResult VTL_validate_interpolation_frames(VTL_Interpolator *interp) {
    if (!interp || !interp->prev_frame || !interp->next_frame || !interp->interpolated_frame) {
        LOG_ERROR("Invalid frame pointers\n");
        return VTL_res_ffmpeg_kInitError;
    }

    if (interp->prev_frame->format != AV_PIX_FMT_YUV420P ||
        interp->next_frame->format != AV_PIX_FMT_YUV420P) {
        LOG_ERROR("Invalid frame formats (expected YUV420P)\n");
        return VTL_res_ffmpeg_kFormatError;
    }

    return VTL_res_kOk;
}

static int VTL_calculate_interpolation_frames_count(VTL_Interpolator *interp) {
    int num_frames = (interp->params.target_frame_rate / interp->params.frame_rate) - 1;
    return num_frames < 1 ? 1 : num_frames;
}

static int64_t VTL_calculate_frame_duration(VTL_Interpolator *interp) {
    int64_t frame_duration = interp->next_frame->pts - interp->prev_frame->pts;
    if (frame_duration <= 0) {
        frame_duration = av_rescale_q(1, (AVRational){1, interp->params.target_frame_rate},
                                    interp->enc_ctx->time_base);
    }
    return frame_duration;
}

static VTL_AppResult VTL_upload_frames_to_gpu(VTL_Interpolator *interp) {
    VTL_AppResult res;
    
    if ((res = VTL_upload_frame_to_gpu(interp, interp->prev_frame, interp->cl_prev_frame)) != VTL_res_kOk ||
        (res = VTL_upload_frame_to_gpu(interp, interp->next_frame, interp->cl_next_frame)) != VTL_res_kOk) {
        return res;
    }
    
    return VTL_res_kOk;
}

static VTL_AppResult VTL_process_interpolated_frame(VTL_Interpolator *interp, 
                                                  float factor,
                                                  int64_t frame_duration) {
    VTL_AppResult res;
    
    for (int plane = 0; plane < 3; plane++) {
        int width = plane == 0 ? interp->params.width : interp->params.width / 2;
        int height = plane == 0 ? interp->params.height : interp->params.height / 2;
        int prev_pitch = interp->prev_frame->linesize[plane];
        int next_pitch = interp->next_frame->linesize[plane];
        int out_pitch = interp->interpolated_frame->linesize[plane];
        
        res = VTL_run_interpolation_kernel(interp, factor, plane, 
                                         width, height,
                                         prev_pitch, next_pitch, out_pitch);
        if (res != VTL_res_kOk) {
            return res;
        }
    }

    if ((res = VTL_download_frame_from_gpu(interp, interp->cl_out_frame, interp->interpolated_frame)) != VTL_res_kOk) {
        return res;
    }

    interp->interpolated_frame->pts = interp->prev_frame->pts + av_rescale_q(
        (int64_t)(frame_duration * factor), 
        interp->dec_ctx->time_base, 
        interp->enc_ctx->time_base);
    interp->interpolated_frame->pkt_dts = interp->interpolated_frame->pts - 1;

    LOG_DEBUG("Interpolated frame pts=%" PRId64 "\n", interp->interpolated_frame->pts);

    return VTL_encode_and_write_frame(interp, interp->interpolated_frame, 0);
}

static VTL_AppResult VTL_interpolate_frame_pair(VTL_Interpolator *interp) {
    VTL_AppResult res = VTL_validate_interpolation_frames(interp);
    if (res != VTL_res_kOk) {
        return res;
    }

    int num_frames = VTL_calculate_interpolation_frames_count(interp);
    int64_t frame_duration = VTL_calculate_frame_duration(interp);

    LOG_DEBUG("Interpolating between frames (generating %d intermediate frames)\n", num_frames);
    LOG_DEBUG("Prev frame pts=%" PRId64 ", Next frame pts=%" PRId64 "\n",
             interp->prev_frame->pts, interp->next_frame->pts);

    if ((res = VTL_upload_frames_to_gpu(interp)) != VTL_res_kOk) {
        return res;
    }

    int i = 1;
    while (i <= num_frames && res == VTL_res_kOk) {
        float factor = (float)i / (num_frames + 1);
        LOG_DEBUG("Generating interpolated frame %d/%d (factor=%.3f)\n", i, num_frames, factor);

        res = VTL_process_interpolated_frame(interp, factor, frame_duration);
        if (res != VTL_res_kOk) {
            LOG_ERROR("Failed to process interpolated frame %d/%d\n", i, num_frames);
        }
        i++;
    }

    return res;
}

VTL_Interpolator *VTL_init_interpolator(const VTL_InterpolationParams *params)
{
    if (!params)
    {
        LOG_ERROR("Invalid parameters\n");
        return NULL;
    }

    LOG_DEBUG("Initializing interpolator with params:\n"
              "  width=%d, height=%d\n"
              "  frame_rate=%d, target_frame_rate=%d\n",
              params->width, params->height,
              params->frame_rate, params->target_frame_rate);

    VTL_Interpolator *interp = calloc(1, sizeof(VTL_Interpolator));
    if (!interp)
    {
        LOG_ERROR("Memory allocation failed\n");
        return NULL;
    }

    interp->params = *params;
    interp->video_stream_index = -1;

    if (VTL_init_opencl(interp) != VTL_res_kOk)
    {
        LOG_ERROR("OpenCL initialization failed\n");
        free(interp);
        return NULL;
    }

    interp->prev_frame = av_frame_alloc();
    interp->next_frame = av_frame_alloc();
    interp->interpolated_frame = av_frame_alloc();

    if (!interp->prev_frame || !interp->next_frame || !interp->interpolated_frame)
    {
        LOG_ERROR("Frame allocation failed\n");
        VTL_free_interpolator(interp);
        return NULL;
    }

    AVFrame *frames[] = {interp->prev_frame, interp->next_frame, interp->interpolated_frame};
    for (int i = 0; i < 3; i++)
    {
        frames[i]->width = params->width;
        frames[i]->height = params->height;
        frames[i]->format = AV_PIX_FMT_YUV420P;
        if (av_frame_get_buffer(frames[i], 32))
        {
            LOG_ERROR("Frame buffer allocation failed\n");
            VTL_free_interpolator(interp);
            return NULL;
        }
    }

    LOG_DEBUG("Interpolator initialized successfully\n");
    return interp;
}

static VTL_AppResult VTL_init_processing_context(VTL_Interpolator *interp,
                                                 const char *input_path,
                                                 AVPacket **pkt,
                                                 AVFrame **frame)
{
    VTL_AppResult res = VTL_res_kOk;

    *pkt = av_packet_alloc();
    *frame = av_frame_alloc();
    if (!*pkt || !*frame)
    {
        LOG_ERROR("Packet/frame allocation failed\n");
        return VTL_res_ffmpeg_kInitError;
    }

    if ((res = VTL_init_video_context(interp, input_path)))
    {
        LOG_ERROR("Input context initialization failed\n");
        return res;
    }

    return res;
}

static VTL_AppResult VTL_process_video_packet(VTL_Interpolator *interp,
                                              AVPacket *pkt,
                                              AVFrame *frame,
                                              int *frame_count,
                                              int64_t *last_pts)
{
    VTL_AppResult res = VTL_res_kOk;

    if (pkt->stream_index != interp->video_stream_index)
    {
        LOG_DEBUG("Skipping non-video packet\n");
        return VTL_res_kOk;
    }

    LOG_DEBUG("Received video packet: pts=%" PRId64 ", dts=%" PRId64 ", size=%d\n",
              pkt->pts, pkt->dts, pkt->size);

    int ret = avcodec_send_packet(interp->dec_ctx, pkt);
    if (ret < 0)
    {
        LOG_ERROR("Error sending packet to decoder: %s\n", av_err2str(ret));
        return VTL_res_ffmpeg_kCodecError;
    }

    return VTL_process_decoded_frames(interp, frame, frame_count, last_pts);
}

static VTL_AppResult VTL_process_decoded_frames(VTL_Interpolator *interp,
                                                AVFrame *frame,
                                                int *frame_count,
                                                int64_t *last_pts)
{
    VTL_AppResult res = VTL_res_kOk;
    int processing_complete = 0;

    while (!processing_complete && res == VTL_res_kOk) {
        int ret = avcodec_receive_frame(interp->dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            processing_complete = 1;
        }
        else if (ret < 0)
        {
            LOG_ERROR("Error receiving frame from decoder: %s\n", av_err2str(ret));
            res = VTL_res_ffmpeg_kCodecError;
        }
        else
        {
            res = VTL_handle_decoded_frame(interp, frame, frame_count, last_pts);
            if (res == VTL_res_kOk)
            {
                (*frame_count)++;
                av_frame_unref(frame);
            }
        }
    }

    return res;
}

static VTL_AppResult VTL_handle_decoded_frame(VTL_Interpolator *interp,
                                              AVFrame *frame,
                                              int *frame_count,
                                              int64_t *last_pts)
{
    LOG_DEBUG("Decoded frame: pts=%" PRId64 ", format=%d, key_frame=%d\n",
              frame->pts, frame->format, frame->key_frame);

    AVFrame *converted = NULL;
    VTL_AppResult res = VTL_convert_frame_to_yuv420p(frame, &converted);
    if (res != VTL_res_kOk)
    {
        LOG_ERROR("Frame conversion failed\n");
        return res;
    }

    if (converted->pts == AV_NOPTS_VALUE)
    {
        converted->pts = *frame_count;
    }

    if (*last_pts != AV_NOPTS_VALUE && converted->pts <= *last_pts)
    {
        converted->pts = *last_pts + 1;
    }
    *last_pts = converted->pts;

    if (!interp->prev_frame->data[0])
    {
        res = VTL_process_first_frame(interp, converted);
    }
    else
    {
        res = VTL_process_intermediate_frame(interp, converted);
    }

    av_frame_free(&converted);
    return res;
}

static VTL_AppResult VTL_process_first_frame(VTL_Interpolator *interp, AVFrame *frame)
{
    av_frame_ref(interp->prev_frame, frame);
    return VTL_encode_and_write_frame(interp, interp->prev_frame, 0);
}

static VTL_AppResult VTL_process_intermediate_frame(VTL_Interpolator *interp, AVFrame *frame)
{
    VTL_AppResult res;

    av_frame_ref(interp->next_frame, frame);

    if ((res = VTL_interpolate_frame_pair(interp)) != VTL_res_kOk)
    {
        LOG_ERROR("Frame interpolation failed\n");
        return res;
    }

    if ((res = VTL_encode_and_write_frame(interp, interp->next_frame, 0)) != VTL_res_kOk)
    {
        LOG_ERROR("Failed to write original frame\n");
        return res;
    }

    AVFrame *tmp = interp->prev_frame;
    interp->prev_frame = interp->next_frame;
    interp->next_frame = tmp;

    return VTL_res_kOk;
}

static VTL_AppResult VTL_flush_decoder(VTL_Interpolator *interp,
                                       AVFrame *frame,
                                       int *frame_count)
{
    VTL_AppResult res = VTL_res_kOk;

    LOG_DEBUG("Flushing decoder\n");
    avcodec_send_packet(interp->dec_ctx, NULL);

    int processing_complete = 0;
    while (!processing_complete && res == VTL_res_kOk)
    {
        int ret = avcodec_receive_frame(interp->dec_ctx, frame);
        if (ret == AVERROR_EOF)
        {
            processing_complete = 1;
        }
        else if (ret < 0)
        {
            LOG_ERROR("Error flushing decoder: %s\n", av_err2str(ret));
            res = VTL_res_ffmpeg_kCodecError;
        }
        else
        {
            res = VTL_process_flush_frame(interp, frame, frame_count);
            if (res == VTL_res_kOk)
            {
                (*frame_count)++;
                av_frame_unref(frame);
            }
        }
    }

    return res;
}

static VTL_AppResult VTL_process_flush_frame(VTL_Interpolator *interp,
                                             AVFrame *frame,
                                             int *frame_count)
{
    AVFrame *converted = NULL;
    VTL_AppResult res = VTL_convert_frame_to_yuv420p(frame, &converted);
    if (res != VTL_res_kOk)
    {
        LOG_ERROR("Frame conversion failed during flush\n");
        return res;
    }

    av_frame_ref(interp->next_frame, converted);

    if ((res = VTL_interpolate_frame_pair(interp)) != VTL_res_kOk)
    {
        LOG_ERROR("Frame interpolation failed during flush\n");
        av_frame_free(&converted);
        return res;
    }

    res = VTL_encode_and_write_frame(interp, interp->next_frame, 0);
    av_frame_free(&converted);

    return res;
}

static VTL_AppResult VTL_cleanup_processing(VTL_Interpolator *interp,
                                            AVPacket *pkt,
                                            AVFrame *frame,
                                            VTL_AppResult res)
{
    if (pkt)
        av_packet_free(&pkt);
    if (frame)
        av_frame_free(&frame);

    if (interp)
    {
        if (interp->dec_ctx)
            avcodec_free_context(&interp->dec_ctx);
        if (interp->enc_ctx)
            avcodec_free_context(&interp->enc_ctx);
        if (interp->fmt_ctx)
            avformat_close_input(&interp->fmt_ctx);
        if (!(interp->out_ctx->oformat->flags & AVFMT_NOFILE) && interp->out_ctx)
        {
            avio_closep(&interp->out_ctx->pb);
            avformat_free_context(interp->out_ctx);
        }
        VTL_free_interpolator(interp);
    }

    if (res == VTL_res_kOk)
    {
        LOG_DEBUG("Video interpolation completed successfully\n");
    }
    else
    {
        LOG_ERROR("Video interpolation failed with error %d\n", res);
    }

    return res;
}

VTL_AppResult VTL_interpolate_video_frames(const char *input_path,
                                           const char *output_path,
                                           const VTL_InterpolationParams *interpol_params)
{
    if (!input_path || !output_path || !interpol_params)
    {
        LOG_ERROR("Invalid parameters\n");
        return VTL_res_ffmpeg_kInitError;
    }

    avformat_network_init();
    LOG_DEBUG("Starting video interpolation:\n"
              "  Input: %s\n  Output: %s\n  Params: %dx%d, %d->%d fps\n",
              input_path, output_path,
              interpol_params->width, interpol_params->height,
              interpol_params->frame_rate, interpol_params->target_frame_rate);

    VTL_Interpolator *interp = VTL_init_interpolator(interpol_params);
    if (!interp)
    {
        LOG_ERROR("Failed to initialize interpolator\n");
        return VTL_res_ffmpeg_kInitError;
    }

    VTL_AppResult res = VTL_res_kOk;
    AVPacket *pkt = NULL;
    AVFrame *frame = NULL;
    int frame_count = 0;
    int64_t last_pts = AV_NOPTS_VALUE;

    if ((res = VTL_init_output_context(interp, output_path)) != VTL_res_kOk)
    {
        LOG_ERROR("Output context initialization failed\n");
        return VTL_cleanup_processing(interp, pkt, frame, res);
    }

    res = VTL_init_processing_context(interp, input_path, &pkt, &frame);
    if (res != VTL_res_kOk)
    {
        return VTL_cleanup_processing(interp, pkt, frame, res);
    }

    LOG_DEBUG("Starting frame processing loop\n");
    int processing_complete = 0;
    while (!processing_complete && res == VTL_res_kOk)
    {
        int ret = av_read_frame(interp->fmt_ctx, pkt);
        if (ret < 0)
        {
            if (ret == AVERROR_EOF)
            {
                LOG_DEBUG("End of input file reached\n");
                processing_complete = 1;
            }
            else
            {
                LOG_ERROR("Error reading frame: %s\n", av_err2str(ret));
                res = VTL_res_ffmpeg_kIOError;
            }
        }
        else
        {
            res = VTL_process_video_packet(interp, pkt, frame, &frame_count, &last_pts);
            av_packet_unref(pkt);
        }
    }

    if (res == VTL_res_kOk)
    {
        res = VTL_flush_decoder(interp, frame, &frame_count);
    }

    if (res == VTL_res_kOk)
    {
        LOG_DEBUG("Flushing encoder\n");
        VTL_encode_and_write_frame(interp, NULL, 1);
        av_write_trailer(interp->out_ctx);
        LOG_DEBUG("Processed %d frames total\n", frame_count);
    }

    return VTL_cleanup_processing(interp, pkt, frame, res);
}

void VTL_free_interpolator(void *interpolator)
{
    if (!interpolator)
        return;

    VTL_Interpolator *interp = (VTL_Interpolator *)interpolator;
    LOG_DEBUG("Freeing interpolator resources\n");

    VTL_cleanup_opencl(interp);

    av_frame_free(&interp->prev_frame);
    av_frame_free(&interp->next_frame);
    av_frame_free(&interp->interpolated_frame);

    if (interp->filter_graph)
        avfilter_graph_free(&interp->filter_graph);

    free(interp);
}