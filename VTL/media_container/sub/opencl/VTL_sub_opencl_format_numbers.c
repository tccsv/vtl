#include <VTL/media_container/sub/opencl/VTL_sub_opencl_format_numbers.h>
#include <OpenCL/opencl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KERNEL_SOURCE
"__kernel void format_numbers(__global const char* in_data, __global int* offsets, __global int* lengths, __global char* out_data, __global int* out_offsets, char sep, int group_len) {\n"
"    int idx = get_global_id(0);\n"
"    int in_offset = offsets[idx];\n"
"    int in_len = lengths[idx];\n"
"    int out_offset = out_offsets[idx];\n"
"    int j = 0;\n"
"    int i = 0;\n"
"    while (i < in_len) {\n"
"        // Копируем нечисловые символы\n"
"        if (in_data[in_offset + i] < '0' || in_data[in_offset + i] > '9') {\n"
"            out_data[out_offset + j++] = in_data[in_offset + i++];\n"
"            continue;\n"
"        }\n"
"        // Найдена последовательность цифр\n"
"        int num_start = i;\n"
"        while (i < in_len && in_data[in_offset + i] >= '0' && in_data[in_offset + i] <= '9') i++;\n"
"        int num_len = i - num_start;\n"
"        // Копируем цифры с разделителями справа налево\n"
"        for (int k = 0; k < num_len; ++k) {\n"
"            if (k > 0 && k % group_len == 0) out_data[out_offset + j++] = sep;\n"
"            out_data[out_offset + j++] = in_data[in_offset + num_start + num_len - 1 - k];\n"
"        }\n"
"        // Разворачиваем обратно\n"
"        for (int k = 0; k < (j - (out_offset + j - num_len - (num_len-1)/group_len)); ++k) {\n"
"            char tmp = out_data[out_offset + j - k - 1];\n"
"            out_data[out_offset + j - k - 1] = out_data[out_offset + j - num_len - (num_len-1)/group_len + k];\n"
"            out_data[out_offset + j - num_len - (num_len-1)/group_len + k] = tmp;\n"
"        }\n"
"    }\n"
"    out_data[out_offset + j] = '\0';\n"
"}\n";

VTL_AppResult VTL_sub_OpenclFormatNumbers(const char** in_texts, char*** out_texts, size_t count, char sep, int group_len) {
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
    program = clCreateProgramWithSource(context, 1, &KERNEL_SOURCE, NULL, &err);
    if (!program || err != CL_SUCCESS) { clReleaseCommandQueue(queue); clReleaseContext(context); return VTL_res_opencl_kProgramError; }
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) { clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); return VTL_res_opencl_kBuildError; }
    kernel = clCreateKernel(program, "format_numbers", &err);
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

    int* out_lengths = (int*)malloc(count * sizeof(int));
    for (size_t i = 0; i < count; ++i) out_lengths[i] = in_lengths[i] * 2 + 1;
    size_t* out_offsets = (size_t*)malloc(count * sizeof(size_t));
    size_t total_out = 0;
    for (size_t i = 0; i < count; ++i) {
        out_offsets[i] = total_out;
        total_out += out_lengths[i];
    }
    char* out_data = (char*)malloc(total_out);

    cl_mem buf_in_data = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, total_in, in_data, &err);
    cl_mem buf_offsets = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, count * sizeof(int), in_offsets, &err);
    cl_mem buf_lengths = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, count * sizeof(int), in_lengths, &err);
    cl_mem buf_out_data = clCreateBuffer(context, CL_MEM_WRITE_ONLY, total_out, NULL, &err);
    cl_mem buf_out_offsets = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, count * sizeof(int), out_offsets, &err);

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_in_data);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_offsets);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &buf_lengths);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), &buf_out_data);
    clSetKernelArg(kernel, 4, sizeof(cl_mem), &buf_out_offsets);
    clSetKernelArg(kernel, 5, sizeof(char), &sep);
    clSetKernelArg(kernel, 6, sizeof(int), &group_len);

    size_t global = count;
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        clReleaseMemObject(buf_in_data);
        clReleaseMemObject(buf_offsets);
        clReleaseMemObject(buf_lengths);
        clReleaseMemObject(buf_out_data);
        clReleaseMemObject(buf_out_offsets);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        free(in_offsets); free(in_lengths); free(in_data); free(out_lengths); free(out_offsets); free(out_data);
        return VTL_res_opencl_kLaunchError;
    }
    clFinish(queue);
    char** texts = (char**)malloc(count * sizeof(char*));
    err = clEnqueueReadBuffer(queue, buf_out_data, CL_TRUE, 0, total_out, out_data, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        clReleaseMemObject(buf_in_data);
        clReleaseMemObject(buf_offsets);
        clReleaseMemObject(buf_lengths);
        clReleaseMemObject(buf_out_data);
        clReleaseMemObject(buf_out_offsets);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        free(in_offsets); free(in_lengths); free(in_data); free(out_lengths); free(out_offsets); free(out_data); free(texts);
        return VTL_res_opencl_kReadBufferError;
    }
    for (size_t i = 0; i < count; ++i) {
        texts[i] = (char*)malloc(out_lengths[i]);
        memcpy(texts[i], out_data + out_offsets[i], out_lengths[i]);
        texts[i][out_lengths[i]-1] = '\0';
    }
    *out_texts = texts;

    clReleaseMemObject(buf_in_data);
    clReleaseMemObject(buf_offsets);
    clReleaseMemObject(buf_lengths);
    clReleaseMemObject(buf_out_data);
    clReleaseMemObject(buf_out_offsets);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    free(in_offsets); free(in_lengths); free(in_data); free(out_lengths); free(out_offsets); free(out_data);
    return VTL_res_kOk;
} 