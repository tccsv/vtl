#include <VTL/media_container/sub/opencl/VTL_sub_opencl_string_stats.h>
#include <OpenCL/opencl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define VTL_SUB_OPENCL_KERNEL_SOURCE_STRING_STATS
"typedef struct {\n"
"    uint length;\n"
"    uint word_count;\n"
"    uint unique_chars;\n"
"    uint char_freq[128];\n"
"} VTL_StringStats_CL;\n"
"__kernel void string_stats(__global const char* in_data, __global int* offsets, __global int* lengths, __global VTL_StringStats_CL* out_stats) {\n"
"    int idx = get_global_id(0);\n"
"    int in_offset = offsets[idx];\n"
"    int in_len = lengths[idx];\n"
"    VTL_StringStats_CL stats = {0, 0, 0, {0}};\n"
"    stats.length = in_len;\n"
"    char prev_space = 1;\n"
"    uchar seen[128] = {0};\n"
"    for (int k = 0; k < in_len; ++k) {\n"
"        char c = in_data[in_offset + k];\n"
"        if ((unsigned char)c < 128) {\n"
"            stats.char_freq[(int)(unsigned char)c]++;\n"
"            if (!seen[(int)(unsigned char)c]) { seen[(int)(unsigned char)c] = 1; stats.unique_chars++; }\n"
"        }\n"
"        if ((c == ' ' || c == '\t' || c == '\n' || c == '\r') && !prev_space) prev_space = 1;\n"
"        else if (!(c == ' ' || c == '\t' || c == '\n' || c == '\r') && prev_space) { stats.word_count++; prev_space = 0; }\n"
"    }\n"
"    out_stats[idx] = stats;\n"
"}\n";

VTL_AppResult VTL_sub_OpenclStringStats(const char** in_texts, VTL_StringStats** out_stats, size_t count) {
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;

    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) return VTL_res_opencl_kPlatformError;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_DEFAULT, 1, &device, NULL);
    if (err != CL_SUCCESS) return VTL_res_opencl_kDeviceError;
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (!context || err != CL_SUCCESS) return VTL_res_opencl_kContextError;
    queue = clCreateCommandQueue(context, device, 0, &err);
    if (!queue || err != CL_SUCCESS) { clReleaseContext(context); return VTL_res_opencl_kQueueError; }
    program = clCreateProgramWithSource(context, 1, &VTL_SUB_OPENCL_KERNEL_SOURCE_STRING_STATS, NULL, &err);
    if (!program || err != CL_SUCCESS) { clReleaseCommandQueue(queue); clReleaseContext(context); return VTL_res_opencl_kProgramError; }
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) { clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); return VTL_res_opencl_kBuildError; }
    kernel = clCreateKernel(program, "string_stats", &err);
    if (!kernel || err != CL_SUCCESS) { clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); return VTL_res_opencl_kKernelError; }

    size_t* in_offsets = (size_t*)malloc(count * sizeof(size_t));
    int* in_lengths = (int*)malloc(count * sizeof(int));
    size_t total_in = 0;
    for (size_t i = 0; i < count; ++i) {
        in_offsets[i] = total_in;
        in_lengths[i] = (int)strlen(in_texts[i]);
        total_in += in_lengths[i];
    }
    char* in_data = (char*)malloc(total_in);
    size_t pos = 0;
    for (size_t i = 0; i < count; ++i) {
        memcpy(in_data + pos, in_texts[i], in_lengths[i]);
        pos += in_lengths[i];
    }

    cl_mem buf_in_data = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, total_in, in_data, &err);
    cl_mem buf_offsets = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, count * sizeof(int), in_offsets, &err);
    cl_mem buf_lengths = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, count * sizeof(int), in_lengths, &err);
    cl_mem buf_out_stats = clCreateBuffer(context, CL_MEM_WRITE_ONLY, count * sizeof(VTL_StringStats), NULL, &err);

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_in_data);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_offsets);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &buf_lengths);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), &buf_out_stats);

    size_t global = count;
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        clReleaseMemObject(buf_in_data);
        clReleaseMemObject(buf_offsets);
        clReleaseMemObject(buf_lengths);
        clReleaseMemObject(buf_out_stats);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        free(in_offsets); free(in_lengths); free(in_data);
        return VTL_res_opencl_kLaunchError;
    }
    clFinish(queue);
    VTL_StringStats* stats = (VTL_StringStats*)malloc(count * sizeof(VTL_StringStats));
    err = clEnqueueReadBuffer(queue, buf_out_stats, CL_TRUE, 0, count * sizeof(VTL_StringStats), stats, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        clReleaseMemObject(buf_in_data);
        clReleaseMemObject(buf_offsets);
        clReleaseMemObject(buf_lengths);
        clReleaseMemObject(buf_out_stats);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        free(in_offsets); free(in_lengths); free(in_data); free(stats);
        return VTL_res_opencl_kReadBufferError;
    }
    *out_stats = stats;

    clReleaseMemObject(buf_in_data);
    clReleaseMemObject(buf_offsets);
    clReleaseMemObject(buf_lengths);
    clReleaseMemObject(buf_out_stats);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    free(in_offsets); free(in_lengths); free(in_data);
    return VTL_res_kOk;
} 