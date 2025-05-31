#include "VTL_img_ops.h"
#include "infra/VTL_imgOpenClKernels.h"
#include "infra/VTL_imgOpenClUtils.h"
#include <CL/cl.h>
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
#include <stdlib.h>
#include <string.h>


// --- AVFrame <-> packed buffer helpers ---
static void avframe_to_packed(const AVFrame* frame, uint8_t* buf, int width, int height, int channels) {
    for (int y = 0; y < height; ++y)
        memcpy(buf + y * width * channels, frame->data[0] + y * frame->linesize[0], width * channels);
}
static void packed_to_avframe(const uint8_t* buf, AVFrame* frame, int width, int height, int channels) {
    for (int y = 0; y < height; ++y)
        memcpy(frame->data[0] + y * frame->linesize[0], buf + y * width * channels, width * channels);
}

// --- Universal OpenCL runner ---
static int run_opencl(const char* src, const uint8_t* in, uint8_t* out, int width, int height, int channels) {
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem buf_in, buf_out;
    size_t img_size = width * height * channels;
    size_t global[2] = {width, height};
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) return -1;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) return -2;
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (!context) return -3;
    queue = clCreateCommandQueue(context, device, 0, &err);
    if (!queue) { clReleaseContext(context); return -4; }
    program = clCreateProgramWithSource(context, 1, &src, NULL, &err);
    if (!program) { clReleaseCommandQueue(queue); clReleaseContext(context); return -5; }
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) { clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); return -6; }
    kernel = clCreateKernel(program, 0, &err);
    if (!kernel) { clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); return -7; }
    buf_in = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, img_size, (void*)in, &err);
    buf_out = clCreateBuffer(context, CL_MEM_WRITE_ONLY, img_size, NULL, &err);
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_in);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_out);
    clSetKernelArg(kernel, 2, sizeof(int), &width);
    clSetKernelArg(kernel, 3, sizeof(int), &height);
    if (channels > 1) clSetKernelArg(kernel, 4, sizeof(int), &channels);
    err = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) { clReleaseMemObject(buf_in); clReleaseMemObject(buf_out); clReleaseKernel(kernel); clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); return -8; }
    clEnqueueReadBuffer(queue, buf_out, CL_TRUE, 0, img_size, out, 0, NULL, NULL);
    clReleaseMemObject(buf_in);
    clReleaseMemObject(buf_out);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    return 0;
}

static int run_opencl_rotate(const char* src, const uint8_t* in, uint8_t* out, int in_w, int in_h, int channels, int angle, int out_w, int out_h) {
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem buf_in, buf_out;
    size_t img_size = out_w * out_h * channels;
    size_t global[2] = {out_w, out_h};
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) return -1;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) return -2;
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (!context) return -3;
    queue = clCreateCommandQueue(context, device, 0, &err);
    if (!queue) { clReleaseContext(context); return -4; }
    program = clCreateProgramWithSource(context, 1, &src, NULL, &err);
    if (!program) { clReleaseCommandQueue(queue); clReleaseContext(context); return -5; }
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) { clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); return -6; }
    kernel = clCreateKernel(program, 0, &err);
    if (!kernel) { clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); return -7; }
    buf_in = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, in_w * in_h * channels, (void*)in, &err);
    buf_out = clCreateBuffer(context, CL_MEM_WRITE_ONLY, img_size, NULL, &err);
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_in);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_out);
    clSetKernelArg(kernel, 2, sizeof(int), &in_w);
    clSetKernelArg(kernel, 3, sizeof(int), &in_h);
    clSetKernelArg(kernel, 4, sizeof(int), &channels);
    clSetKernelArg(kernel, 5, sizeof(int), &angle);
    clSetKernelArg(kernel, 6, sizeof(int), &out_w);
    clSetKernelArg(kernel, 7, sizeof(int), &out_h);
    err = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) { clReleaseMemObject(buf_in); clReleaseMemObject(buf_out); clReleaseKernel(kernel); clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); return -8; }
    clEnqueueReadBuffer(queue, buf_out, CL_TRUE, 0, img_size, out, 0, NULL, NULL);
    clReleaseMemObject(buf_in);
    clReleaseMemObject(buf_out);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    return 0;
}

static int run_opencl_flip(const char* src, const uint8_t* in, uint8_t* out, int width, int height, int channels, int vertical) {
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem buf_in, buf_out;
    size_t img_size = width * height * channels;
    size_t global[2] = {width, height};
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) return -1;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) return -2;
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (!context) return -3;
    queue = clCreateCommandQueue(context, device, 0, &err);
    if (!queue) { clReleaseContext(context); return -4; }
    program = clCreateProgramWithSource(context, 1, &src, NULL, &err);
    if (!program) { clReleaseCommandQueue(queue); clReleaseContext(context); return -5; }
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) { clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); return -6; }
    kernel = clCreateKernel(program, 0, &err);
    if (!kernel) { clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); return -7; }
    buf_in = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, img_size, (void*)in, &err);
    buf_out = clCreateBuffer(context, CL_MEM_WRITE_ONLY, img_size, NULL, &err);
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_in);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_out);
    clSetKernelArg(kernel, 2, sizeof(int), &width);
    clSetKernelArg(kernel, 3, sizeof(int), &height);
    clSetKernelArg(kernel, 4, sizeof(int), &channels);
    clSetKernelArg(kernel, 5, sizeof(int), &vertical);
    err = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) { clReleaseMemObject(buf_in); clReleaseMemObject(buf_out); clReleaseKernel(kernel); clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); return -8; }
    clEnqueueReadBuffer(queue, buf_out, CL_TRUE, 0, img_size, out, 0, NULL, NULL);
    clReleaseMemObject(buf_in);
    clReleaseMemObject(buf_out);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    return 0;
}

static int run_opencl_overlay(const char* src, const uint8_t* base, const uint8_t* over, uint8_t* out, int width, int height, int channels, float alpha) {
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem buf_base, buf_over, buf_out;
    size_t img_size = width * height * channels;
    size_t global[2] = {width, height};
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) return -1;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) return -2;
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (!context) return -3;
    queue = clCreateCommandQueue(context, device, 0, &err);
    if (!queue) { clReleaseContext(context); return -4; }
    program = clCreateProgramWithSource(context, 1, &src, NULL, &err);
    if (!program) { clReleaseCommandQueue(queue); clReleaseContext(context); return -5; }
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) { clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); return -6; }
    kernel = clCreateKernel(program, 0, &err);
    if (!kernel) { clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); return -7; }
    buf_base = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, img_size, (void*)base, &err);
    buf_over = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, img_size, (void*)over, &err);
    buf_out = clCreateBuffer(context, CL_MEM_WRITE_ONLY, img_size, NULL, &err);
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_base);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_over);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &buf_out);
    clSetKernelArg(kernel, 3, sizeof(int), &width);
    clSetKernelArg(kernel, 4, sizeof(int), &height);
    clSetKernelArg(kernel, 5, sizeof(int), &channels);
    clSetKernelArg(kernel, 6, sizeof(float), &alpha);
    err = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) { clReleaseMemObject(buf_base); clReleaseMemObject(buf_over); clReleaseMemObject(buf_out); clReleaseKernel(kernel); clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); return -8; }
    clEnqueueReadBuffer(queue, buf_out, CL_TRUE, 0, img_size, out, 0, NULL, NULL);
    clReleaseMemObject(buf_base);
    clReleaseMemObject(buf_over);
    clReleaseMemObject(buf_out);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    return 0;
}

// --- Operation wrappers ---
#define ALLOC_OUT_FRAME(channels, fmt) \
    *out = av_frame_alloc(); \
    av_frame_copy_props(*out, in); \
    (*out)->width = width; \
    (*out)->height = height; \
    (*out)->format = fmt; \
    av_frame_get_buffer(*out, 32);

int VTL_img_EdgeDetect(AVFrame* in, AVFrame** out, int width, int height, enum AVPixelFormat pix_fmt) {
    int channels = (pix_fmt == AV_PIX_FMT_RGB24) ? 3 : 1;
    ALLOC_OUT_FRAME(channels, pix_fmt);
    uint8_t* inbuf = malloc(width * height * channels);
    uint8_t* outbuf = malloc(width * height * channels);
    avframe_to_packed(in, inbuf, width, height, channels);
    int ret = run_opencl(VTL_IMG_KERNEL_EDGE, inbuf, outbuf, width, height, channels);
    packed_to_avframe(outbuf, *out, width, height, channels);
    free(inbuf); free(outbuf);
    return ret;
}

int VTL_img_Sharpen(AVFrame* in, AVFrame** out, int width, int height, enum AVPixelFormat pix_fmt) {
    int channels = (pix_fmt == AV_PIX_FMT_RGB24) ? 3 : 1;
    ALLOC_OUT_FRAME(channels, pix_fmt);
    uint8_t* inbuf = malloc(width * height * channels);
    uint8_t* outbuf = malloc(width * height * channels);
    avframe_to_packed(in, inbuf, width, height, channels);
    int ret = run_opencl(VTL_IMG_KERNEL_SHARPEN, inbuf, outbuf, width, height, channels);
    packed_to_avframe(outbuf, *out, width, height, channels);
    free(inbuf); free(outbuf);
    return ret;
}

int VTL_img_Grayscale(AVFrame* in, AVFrame** out, int width, int height, enum AVPixelFormat pix_fmt) {
    if (pix_fmt != AV_PIX_FMT_RGB24) return -100;
    *out = av_frame_alloc();
    av_frame_copy_props(*out, in);
    (*out)->width = width;
    (*out)->height = height;
    (*out)->format = AV_PIX_FMT_GRAY8;
    av_frame_get_buffer(*out, 32);
    uint8_t* inbuf = malloc(width * height * 3);
    uint8_t* outbuf = malloc(width * height);
    avframe_to_packed(in, inbuf, width, height, 3);
    int ret = run_opencl(VTL_IMG_KERNEL_GRAYSCALE, inbuf, outbuf, width, height, 1);
    packed_to_avframe(outbuf, *out, width, height, 1);
    free(inbuf); free(outbuf);
    return ret;
}

int VTL_img_Invert(AVFrame* in, AVFrame** out, int width, int height, enum AVPixelFormat pix_fmt) {
    int channels = (pix_fmt == AV_PIX_FMT_RGB24) ? 3 : 1;
    ALLOC_OUT_FRAME(channels, pix_fmt);
    uint8_t* inbuf = malloc(width * height * channels);
    uint8_t* outbuf = malloc(width * height * channels);
    avframe_to_packed(in, inbuf, width, height, channels);
    int ret = run_opencl(VTL_IMG_KERNEL_INVERT, inbuf, outbuf, width, height, channels);
    packed_to_avframe(outbuf, *out, width, height, channels);
    free(inbuf); free(outbuf);
    return ret;
}

int VTL_img_HighPass(AVFrame* in, AVFrame** out, int width, int height, enum AVPixelFormat pix_fmt) {
    int channels = (pix_fmt == AV_PIX_FMT_RGB24) ? 3 : 1;
    ALLOC_OUT_FRAME(channels, pix_fmt);
    uint8_t* inbuf = malloc(width * height * channels);
    uint8_t* outbuf = malloc(width * height * channels);
    avframe_to_packed(in, inbuf, width, height, channels);
    int ret = run_opencl(VTL_IMG_KERNEL_HIGHPASS, inbuf, outbuf, width, height, channels);
    packed_to_avframe(outbuf, *out, width, height, channels);
    free(inbuf); free(outbuf);
    return ret;
}

int VTL_img_Rotate(AVFrame* in, AVFrame** out, int width, int height, enum AVPixelFormat pix_fmt, int angle) {
    int channels = (pix_fmt == AV_PIX_FMT_RGB24) ? 3 : 1;
    int out_w = width, out_h = height;
    if (angle == 90 || angle == 270) {
        out_w = height;
        out_h = width;
    }
    *out = av_frame_alloc();
    av_frame_copy_props(*out, in);
    (*out)->width = out_w;
    (*out)->height = out_h;
    (*out)->format = pix_fmt;
    av_frame_get_buffer(*out, 32);
    uint8_t* inbuf = malloc(width * height * channels);
    uint8_t* outbuf = malloc(out_w * out_h * channels);
    avframe_to_packed(in, inbuf, width, height, channels);
    int ret = run_opencl_rotate(VTL_IMG_KERNEL_ROTATE, inbuf, outbuf, width, height, channels, angle, out_w, out_h);
    packed_to_avframe(outbuf, *out, out_w, out_h, channels);
    free(inbuf); free(outbuf);
    return ret;
}

int VTL_img_flip(AVFrame* in, AVFrame** out, int width, int height, enum AVPixelFormat pix_fmt, int vertical) {
    int channels = (pix_fmt == AV_PIX_FMT_RGB24) ? 3 : 1;
    ALLOC_OUT_FRAME(channels, pix_fmt);
    uint8_t* inbuf = malloc(width * height * channels);
    uint8_t* outbuf = malloc(width * height * channels);
    avframe_to_packed(in, inbuf, width, height, channels);
    int ret = run_opencl_flip(VTL_IMG_KERNEL_FLIP, inbuf, outbuf, width, height, channels, vertical);
    packed_to_avframe(outbuf, *out, width, height, channels);
    free(inbuf); free(outbuf);
    return ret;
}

int VTL_img_overlay(AVFrame* base, AVFrame* overlay, AVFrame** out, int width, int height, enum AVPixelFormat pix_fmt, float alpha) {
    int channels = (pix_fmt == AV_PIX_FMT_RGB24) ? 3 : 1;
    *out = av_frame_alloc();
    av_frame_copy_props(*out, base);
    (*out)->width = width;
    (*out)->height = height;
    (*out)->format = pix_fmt;
    av_frame_get_buffer(*out, 32);
    uint8_t* basebuf = malloc(width * height * channels);
    uint8_t* overbuf = malloc(width * height * channels);
    uint8_t* outbuf = malloc(width * height * channels);
    avframe_to_packed(base, basebuf, width, height, channels);
    avframe_to_packed(overlay, overbuf, width, height, channels);
    int ret = run_opencl_overlay(VTL_IMG_KERNEL_OVERLAY, basebuf, overbuf, outbuf, width, height, channels, alpha);
    packed_to_avframe(outbuf, *out, width, height, channels);
    free(basebuf); free(overbuf); free(outbuf);
    return ret;
} 