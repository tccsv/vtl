#include <CL/cl.h>
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VTL_IMG_KERNEL_BLUR \
    "__kernel void blur3x3(__global const uchar* in, __global uchar* out, int width, int height) {\n" \
    "    int x = get_global_id(0);\n" \
    "    int y = get_global_id(1);\n" \
    "    int idx = y * width + x;\n" \
    "    if (x > 0 && y > 0 && x < width-1 && y < height-1) {\n" \
    "        int sum = 0;\n" \
    "        for (int dy = -1; dy <= 1; ++dy)\n" \
    "            for (int dx = -1; dx <= 1; ++dx)\n" \
    "                sum += in[(y+dy)*width + (x+dx)];\n" \
    "        out[idx] = sum / 9;\n" \
    "    } else {\n" \
    "        out[idx] = in[idx];\n" \
    "    }\n" \
    "}\n"

#define VTL_IMG_KERNEL_BLUR_RGB \
    "__kernel void blur3x3_rgb(__global const uchar* in, __global uchar* out, int width, int height) {\n" \
    "    int x = get_global_id(0);\n" \
    "    int y = get_global_id(1);\n" \
    "    int idx = (y * width + x) * 3;\n" \
    "    if (x > 0 && y > 0 && x < width-1 && y < height-1) {\n" \
    "        for (int c = 0; c < 3; ++c) {\n" \
    "            int sum = 0;\n" \
    "            for (int dy = -1; dy <= 1; ++dy)\n" \
    "                for (int dx = -1; dx <= 1; ++dx)\n" \
    "                    sum += in[((y+dy)*width + (x+dx))*3 + c];\n" \
    "            out[idx + c] = sum / 9;\n" \
    "        }\n" \
    "    } else {\n" \
    "        out[idx + 0] = in[idx + 0];\n" \
    "        out[idx + 1] = in[idx + 1];\n" \
    "        out[idx + 2] = in[idx + 2];\n" \
    "    }\n" \
    "}\n"

#define VTL_IMG_KERNEL_ROTATE \
    "__kernel void rotate(__global const uchar* in, __global uchar* out, int in_w, int in_h, int channels, int angle, int out_w, int out_h) {\n" \
    "    int x = get_global_id(0);\n" \
    "    int y = get_global_id(1);\n" \
    "    int nx = x, ny = y;\n" \
    "    if (angle == 90) { nx = y; ny = in_w - 1 - x; }\n" \
    "    else if (angle == 180) { nx = in_w - 1 - x; ny = in_h - 1 - y; }\n" \
    "    else if (angle == 270) { nx = in_h - 1 - y; ny = x; }\n" \
    "    if (nx >= 0 && nx < out_w && ny >= 0 && ny < out_h) {\n" \
    "        int src_idx = (y * in_w + x) * channels;\n" \
    "        int dst_idx = (ny * out_w + nx) * channels;\n" \
    "        for (int c = 0; c < channels; ++c) out[dst_idx + c] = in[src_idx + c];\n" \
    "    }\n" \
    "}\n"

#define VTL_IMG_KERNEL_INVERT \
    "__kernel void invert(__global const uchar* in, __global uchar* out, int width, int height, int channels) {\n" \
    "    int x = get_global_id(0);\n" \
    "    int y = get_global_id(1);\n" \
    "    int idx = (y * width + x) * channels;\n" \
    "    for (int c = 0; c < channels; ++c)\n" \
    "        out[idx+c] = 255 - in[idx+c];\n" \
    "}\n"

#define VTL_IMG_KERNEL_EDGE \
    "__kernel void edge(__global const uchar* in, __global uchar* out, int width, int height, int channels) {\n" \
    "    int x = get_global_id(0);\n" \
    "    int y = get_global_id(1);\n" \
    "    int idx = (y * width + x) * channels;\n" \
    "    int gx[3] = {0,0,0}, gy[3] = {0,0,0};\n" \
    "    int sobel_x[3][3] = { {-1,0,1},{-2,0,2},{-1,0,1} };\n" \
    "    int sobel_y[3][3] = { {-1,-2,-1},{0,0,0},{1,2,1} };\n" \
    "    for (int c = 0; c < channels; ++c) {\n" \
    "        if (x > 0 && y > 0 && x < width-1 && y < height-1) {\n" \
    "            gx[c] = 0; gy[c] = 0;\n" \
    "            for (int dy = -1; dy <= 1; ++dy)\n" \
    "                for (int dx = -1; dx <= 1; ++dx) {\n" \
    "                    int v = in[((y+dy)*width + (x+dx))*channels + c];\n" \
    "                    gx[c] += sobel_x[dy+1][dx+1]*v;\n" \
    "                    gy[c] += sobel_y[dy+1][dx+1]*v;\n" \
    "                }\n" \
    "            int mag = (int)hypot((float)gx[c], (float)gy[c]);\n" \
    "            out[idx+c] = mag > 255 ? 255 : mag;\n" \
    "        } else { out[idx+c] = 0; }\n" \
    "    }\n" \
    "}\n"

static int run_opencl_blur(const uint8_t* in, uint8_t* out, int width, int height) {
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem buf_in, buf_out;
    size_t img_size = width * height;
    size_t global[2] = {width, height};
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) return -1;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) return -2;
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (!context) return -3;
    queue = clCreateCommandQueue(context, device, 0, &err);
    if (!queue) { clReleaseContext(context); return -4; }
    const char* src = VTL_IMG_KERNEL_BLUR;
    program = clCreateProgramWithSource(context, 1, &src, NULL, &err);
    if (!program) { clReleaseCommandQueue(queue); clReleaseContext(context); return -5; }
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) { clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); return -6; }
    kernel = clCreateKernel(program, "blur3x3", &err);
    if (!kernel) { clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); return -7; }
    buf_in = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, img_size, (void*)in, &err);
    buf_out = clCreateBuffer(context, CL_MEM_WRITE_ONLY, img_size, NULL, &err);
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_in);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_out);
    clSetKernelArg(kernel, 2, sizeof(int), &width);
    clSetKernelArg(kernel, 3, sizeof(int), &height);
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

static int run_opencl_blur_rgb(const uint8_t* in, uint8_t* out, int width, int height) {
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem buf_in, buf_out;
    size_t img_size = width * height * 3;
    size_t global[2] = {width, height};
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) return -1;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) return -2;
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (!context) return -3;
    queue = clCreateCommandQueue(context, device, 0, &err);
    if (!queue) { clReleaseContext(context); return -4; }
    const char* src = VTL_IMG_KERNEL_BLUR_RGB;
    program = clCreateProgramWithSource(context, 1, &src, NULL, &err);
    if (!program) { clReleaseCommandQueue(queue); clReleaseContext(context); return -5; }
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) { clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); return -6; }
    kernel = clCreateKernel(program, "blur3x3_rgb", &err);
    if (!kernel) { clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); return -7; }
    buf_in = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, img_size, (void*)in, &err);
    buf_out = clCreateBuffer(context, CL_MEM_WRITE_ONLY, img_size, NULL, &err);
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_in);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_out);
    clSetKernelArg(kernel, 2, sizeof(int), &width);
    clSetKernelArg(kernel, 3, sizeof(int), &height);
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

static int run_opencl_rotate(const uint8_t* in, uint8_t* out, int in_w, int in_h, int channels, int angle, int out_w, int out_h) {
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
    const char* src = VTL_IMG_KERNEL_ROTATE;
    program = clCreateProgramWithSource(context, 1, &src, NULL, &err);
    if (!program) { clReleaseCommandQueue(queue); clReleaseContext(context); return -5; }
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) { clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); return -6; }
    kernel = clCreateKernel(program, "rotate", &err);
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

static int run_opencl_invert(const uint8_t* in, uint8_t* out, int width, int height, int channels) {
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
    const char* src = VTL_IMG_KERNEL_INVERT;
    program = clCreateProgramWithSource(context, 1, &src, NULL, &err);
    if (!program) { clReleaseCommandQueue(queue); clReleaseContext(context); return -5; }
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) { clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); return -6; }
    kernel = clCreateKernel(program, "invert", &err);
    if (!kernel) { clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); return -7; }
    buf_in = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, img_size, (void*)in, &err);
    buf_out = clCreateBuffer(context, CL_MEM_WRITE_ONLY, img_size, NULL, &err);
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_in);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_out);
    clSetKernelArg(kernel, 2, sizeof(int), &width);
    clSetKernelArg(kernel, 3, sizeof(int), &height);
    clSetKernelArg(kernel, 4, sizeof(int), &channels);
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

static int run_opencl_edge(const uint8_t* in, uint8_t* out, int width, int height, int channels) {
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
    const char* src = VTL_IMG_KERNEL_EDGE;
    program = clCreateProgramWithSource(context, 1, &src, NULL, &err);
    if (!program) { clReleaseCommandQueue(queue); clReleaseContext(context); return -5; }
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) { clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); return -6; }
    kernel = clCreateKernel(program, "edge", &err);
    if (!kernel) { clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); return -7; }
    buf_in = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, img_size, (void*)in, &err);
    buf_out = clCreateBuffer(context, CL_MEM_WRITE_ONLY, img_size, NULL, &err);
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_in);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_out);
    clSetKernelArg(kernel, 2, sizeof(int), &width);
    clSetKernelArg(kernel, 3, sizeof(int), &height);
    clSetKernelArg(kernel, 4, sizeof(int), &channels);
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

int VTL_img_blur(AVFrame* in_frame, AVFrame** out_frame, int width, int height, enum AVPixelFormat pix_fmt, const char* blur_params) {
    if (pix_fmt == AV_PIX_FMT_GRAY8) {
        *out_frame = av_frame_alloc();
        av_frame_copy_props(*out_frame, in_frame);
        (*out_frame)->width = width;
        (*out_frame)->height = height;
        (*out_frame)->format = pix_fmt;
        av_frame_get_buffer(*out_frame, 32);
        int ret = run_opencl_blur(in_frame->data[0], (*out_frame)->data[0], width, height);
        return ret;
    } else if (pix_fmt == AV_PIX_FMT_RGB24) {
        *out_frame = av_frame_alloc();
        av_frame_copy_props(*out_frame, in_frame);
        (*out_frame)->width = width;
        (*out_frame)->height = height;
        (*out_frame)->format = pix_fmt;
        av_frame_get_buffer(*out_frame, 32);
        int ret = run_opencl_blur_rgb(in_frame->data[0], (*out_frame)->data[0], width, height);
        return ret;
    } else {
        return -100; // Unsupported format
    }
}

int VTL_img_rotate(AVFrame* in_frame, AVFrame** out_frame, int width, int height, enum AVPixelFormat pix_fmt, int angle) {
    int channels = (pix_fmt == AV_PIX_FMT_RGB24) ? 3 : 1;
    int out_w = width, out_h = height;
    if (angle == 90 || angle == 270) {
        out_w = height;
        out_h = width;
    }
    *out_frame = av_frame_alloc();
    av_frame_copy_props(*out_frame, in_frame);
    (*out_frame)->width = out_w;
    (*out_frame)->height = out_h;
    (*out_frame)->format = pix_fmt;
    av_frame_get_buffer(*out_frame, 32);
    int img_size = width * height * channels;
    int out_img_size = out_w * out_h * channels;
    uint8_t* inbuf = malloc(img_size);
    uint8_t* outbuf = malloc(out_img_size);
    memcpy(inbuf, in_frame->data[0], img_size);
    int ret = run_opencl_rotate(inbuf, outbuf, width, height, channels, angle, out_w, out_h);
    memcpy((*out_frame)->data[0], outbuf, out_img_size);
    free(inbuf);
    free(outbuf);
    return ret;
}

int VTL_img_invert(AVFrame* in_frame, AVFrame** out_frame, int width, int height, enum AVPixelFormat pix_fmt) {
    int channels = (pix_fmt == AV_PIX_FMT_RGB24) ? 3 : 1;
    *out_frame = av_frame_alloc();
    av_frame_copy_props(*out_frame, in_frame);
    (*out_frame)->width = width;
    (*out_frame)->height = height;
    (*out_frame)->format = pix_fmt;
    av_frame_get_buffer(*out_frame, 32);
    int img_size = width * height * channels;
    uint8_t* inbuf = malloc(img_size);
    uint8_t* outbuf = malloc(img_size);
    memcpy(inbuf, in_frame->data[0], img_size);
    int ret = run_opencl_invert(inbuf, outbuf, width, height, channels);
    memcpy((*out_frame)->data[0], outbuf, img_size);
    free(inbuf);
    free(outbuf);
    return ret;
}

int VTL_img_edge_detect(AVFrame* in_frame, AVFrame** out_frame, int width, int height, enum AVPixelFormat pix_fmt) {
    int channels = (pix_fmt == AV_PIX_FMT_RGB24) ? 3 : 1;
    *out_frame = av_frame_alloc();
    av_frame_copy_props(*out_frame, in_frame);
    (*out_frame)->width = width;
    (*out_frame)->height = height;
    (*out_frame)->format = pix_fmt;
    av_frame_get_buffer(*out_frame, 32);
    int img_size = width * height * channels;
    uint8_t* inbuf = malloc(img_size);
    uint8_t* outbuf = malloc(img_size);
    memcpy(inbuf, in_frame->data[0], img_size);
    int ret = run_opencl_edge(inbuf, outbuf, width, height, channels);
    memcpy((*out_frame)->data[0], outbuf, img_size);
    free(inbuf);
    free(outbuf);
    return ret;
} 