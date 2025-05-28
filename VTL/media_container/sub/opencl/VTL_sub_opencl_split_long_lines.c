#include <VTL/media_container/sub/opencl/VTL_sub_opencl_split_long_lines.h>
#include <OpenCL/opencl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// OpenCL-ядро: разбивает строку на подстроки не длиннее max_len, стараясь не разрывать слова
const char* kernelSource =
"__kernel void split_long_lines(__global const char* in_data, __global int* offsets, __global int* lengths, __global int* max_len, __global int* out_offsets, __global int* out_lengths, __global int* out_counts) {\n"
"    int idx = get_global_id(0);\n"
"    int in_offset = offsets[idx];\n"
"    int in_len = lengths[idx];\n"
"    int mlen = max_len[idx];\n"
"    int out_off = out_offsets[idx];\n"
"    int out_count = 0;\n"
"    int pos = 0;\n"
"    while (pos < in_len) {\n"
"        int end = pos + mlen;\n"
"        if (end > in_len) end = in_len;\n"
"        int split = end;\n"
"        // ищем последний пробел до split\n"
"        for (int i = end - 1; i > pos; --i) {\n"
"            if (in_data[in_offset + i] == ' ') { split = i; break; }\n"
"        }\n"
"        if (split == pos) split = end; // если пробела нет, режем по max_len\n"
"        out_lengths[out_off + out_count] = split - pos;\n"
"        out_count++;\n"
"        pos = split;\n"
"        while (pos < in_len && in_data[in_offset + pos] == ' ') pos++; // пропускаем пробелы\n"
"    }\n"
"    out_counts[idx] = out_count;\n"
"}\n";

VTL_AppResult VTL_sub_OpenclSplitLongLines(const char** in_texts, char**** out_texts, int** out_counts, size_t count, int max_len) {
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;

    // Подготовка входных данных
    size_t* in_offsets = (size_t*)malloc(count * sizeof(size_t));
    int* in_lengths = (int*)malloc(count * sizeof(int));
    int* max_lens = (int*)malloc(count * sizeof(int));
    size_t total_in = 0;
    for (size_t i = 0; i < count; ++i) {
        in_offsets[i] = total_in;
        in_lengths[i] = (int)strlen(in_texts[i]);
        max_lens[i] = max_len;
        total_in += in_lengths[i];
    }
    char* in_data = (char*)malloc(total_in);
    size_t pos = 0;
    for (size_t i = 0; i < count; ++i) {
        memcpy(in_data + pos, in_texts[i], in_lengths[i]);
        pos += in_lengths[i];
    }

    // Оценка максимального количества строк на одну входную (грубо: длина/max_len + 1)
    int* max_splits = (int*)malloc(count * sizeof(int));
    int total_splits = 0;
    for (size_t i = 0; i < count; ++i) {
        max_splits[i] = in_lengths[i] / max_len + 2;
        total_splits += max_splits[i];
    }

    // Буферы для выходных длин и смещений
    int* out_lengths = (int*)calloc(total_splits, sizeof(int));
    int* out_offsets = (int*)malloc(count * sizeof(int));
    int* out_counts_arr = (int*)calloc(count, sizeof(int));
    int cur_offset = 0;
    for (size_t i = 0; i < count; ++i) {
        out_offsets[i] = cur_offset;
        cur_offset += max_splits[i];
    }

    // OpenCL init
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) goto cleanup; 
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_DEFAULT, 1, &device, NULL);
    if (err != CL_SUCCESS) goto cleanup;
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (!context || err != CL_SUCCESS) goto cleanup;
    queue = clCreateCommandQueue(context, device, 0, &err);
    if (!queue || err != CL_SUCCESS) { clReleaseContext(context); goto cleanup; }
    program = clCreateProgramWithSource(context, 1, &kernelSource, NULL, &err);
    if (!program || err != CL_SUCCESS) { clReleaseCommandQueue(queue); clReleaseContext(context); goto cleanup; }
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) { clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); goto cleanup; }
    kernel = clCreateKernel(program, "split_long_lines", &err);
    if (!kernel || err != CL_SUCCESS) { clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); goto cleanup; }

    cl_mem buf_in_data = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, total_in, in_data, &err);
    cl_mem buf_offsets = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, count * sizeof(int), in_offsets, &err);
    cl_mem buf_lengths = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, count * sizeof(int), in_lengths, &err);
    cl_mem buf_max_len = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, count * sizeof(int), max_lens, &err);
    cl_mem buf_out_offsets = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, count * sizeof(int), out_offsets, &err);
    cl_mem buf_out_lengths = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, total_splits * sizeof(int), out_lengths, &err);
    cl_mem buf_out_counts = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, count * sizeof(int), out_counts_arr, &err);

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_in_data);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_offsets);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &buf_lengths);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), &buf_max_len);
    clSetKernelArg(kernel, 4, sizeof(cl_mem), &buf_out_offsets);
    clSetKernelArg(kernel, 5, sizeof(cl_mem), &buf_out_lengths);
    clSetKernelArg(kernel, 6, sizeof(cl_mem), &buf_out_counts);

    size_t global = count;
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) goto cleanup_cl;
    clFinish(queue);

    err = clEnqueueReadBuffer(queue, buf_out_lengths, CL_TRUE, 0, total_splits * sizeof(int), out_lengths, 0, NULL, NULL);
    err |= clEnqueueReadBuffer(queue, buf_out_counts, CL_TRUE, 0, count * sizeof(int), out_counts_arr, 0, NULL, NULL);
    if (err != CL_SUCCESS) goto cleanup_cl;

    // Формируем выходной массив массивов строк
    char*** result = (char***)malloc(count * sizeof(char**));
    int* result_counts = (int*)malloc(count * sizeof(int));
    char* src;
    for (size_t i = 0; i < count; ++i) {
        int n = out_counts_arr[i];
        result_counts[i] = n;
        result[i] = (char**)malloc(n * sizeof(char*));
        int offset = out_offsets[i];
        int in_offset = in_offsets[i];
        int pos = 0;
        for (int j = 0; j < n; ++j) {
            int len = out_lengths[offset + j];
            result[i][j] = (char*)malloc(len + 1);
            memcpy(result[i][j], in_texts[i] + pos, len);
            result[i][j][len] = '\0';
            pos += len;
            while (in_texts[i][pos] == ' ') pos++; // пропуск пробелов
        }
    }
    *out_texts = result;
    *out_counts = result_counts;

cleanup_cl:
    clReleaseMemObject(buf_in_data);
    clReleaseMemObject(buf_offsets);
    clReleaseMemObject(buf_lengths);
    clReleaseMemObject(buf_max_len);
    clReleaseMemObject(buf_out_offsets);
    clReleaseMemObject(buf_out_lengths);
    clReleaseMemObject(buf_out_counts);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
cleanup:
    free(in_offsets); free(in_lengths); free(max_lens); free(in_data); free(max_splits); free(out_lengths); free(out_offsets); free(out_counts_arr);
    return VTL_res_kOk;
} 