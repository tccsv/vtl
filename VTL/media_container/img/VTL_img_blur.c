#include <CL/cl.h>
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* blur_kernel_src =
    "__kernel void blur3x3(__global const uchar* in, __global uchar* out, int width, int height) {\n"
    "    int x = get_global_id(0);\n"
    "    int y = get_global_id(1);\n"
    "    int idx = y * width + x;\n"
    "    if (x > 0 && y > 0 && x < width-1 && y < height-1) {\n"
    "        int sum = 0;\n"
    "        for (int dy = -1; dy <= 1; ++dy)\n"
    "            for (int dx = -1; dx <= 1; ++dx)\n"
    "                sum += in[(y+dy)*width + (x+dx)];\n"
    "        out[idx] = sum / 9;\n"
    "    } else {\n"
    "        out[idx] = in[idx];\n"
    "    }\n"
    "}\n";

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
    program = clCreateProgramWithSource(context, 1, &blur_kernel_src, NULL, &err);
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

static const char* blur_kernel_src_rgb =
    "__kernel void blur3x3_rgb(__global const uchar* in, __global uchar* out, int width, int height) {\n"
    "    int x = get_global_id(0);\n"
    "    int y = get_global_id(1);\n"
    "    int idx = (y * width + x) * 3;\n"
    "    if (x > 0 && y > 0 && x < width-1 && y < height-1) {\n"
    "        for (int c = 0; c < 3; ++c) {\n"
    "            int sum = 0;\n"
    "            for (int dy = -1; dy <= 1; ++dy)\n"
    "                for (int dx = -1; dx <= 1; ++dx)\n"
    "                    sum += in[((y+dy)*width + (x+dx))*3 + c];\n"
    "            out[idx + c] = sum / 9;\n"
    "        }\n"
    "    } else {\n"
    "        out[idx + 0] = in[idx + 0];\n"
    "        out[idx + 1] = in[idx + 1];\n"
    "        out[idx + 2] = in[idx + 2];\n"
    "    }\n"
    "}\n";

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
    program = clCreateProgramWithSource(context, 1, &blur_kernel_src_rgb, NULL, &err);
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