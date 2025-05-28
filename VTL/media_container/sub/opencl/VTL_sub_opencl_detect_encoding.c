#include <VTL/media_container/sub/opencl/VTL_sub_opencl_detect_encoding.h>
#include <OpenCL/opencl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// OpenCL-ядро для определения кодировки строки
const char* kernelSource =
"__kernel void detect_encoding(__global const char* in_data, __global int* offsets, __global int* lengths, __global int* out_encodings) {\n"
"    int idx = get_global_id(0);\n"
"    int in_offset = offsets[idx];\n"
"    int in_len = lengths[idx];\n"
"    int encoding = 4; // неизвестно\n"
"    int ascii = 1;\n"
"    int latin1 = 1;\n"
"    int win1251 = 1;\n"
"    int utf8 = 1;\n"
"    int i = 0;\n"
"    while (i < in_len) {\n"
"        unsigned char c = in_data[in_offset + i];\n"
"        if (c < 0x80) { i++; continue; }\n"
"        ascii = 0;\n"
"        // UTF-8 check\n"
"        if ((c & 0xE0) == 0xC0 && i+1 < in_len && (in_data[in_offset+i+1] & 0xC0) == 0x80) { i+=2; continue; }\n"
"        if ((c & 0xF0) == 0xE0 && i+2 < in_len && (in_data[in_offset+i+1] & 0xC0) == 0x80 && (in_data[in_offset+i+2] & 0xC0) == 0x80) { i+=3; continue; }\n"
"        if ((c & 0xF8) == 0xF0 && i+3 < in_len && (in_data[in_offset+i+1] & 0xC0) == 0x80 && (in_data[in_offset+i+2] & 0xC0) == 0x80 && (in_data[in_offset+i+3] & 0xC0) == 0x80) { i+=4; continue; }\n"
"        utf8 = 0;\n"
"        // Latin1 check: 0xA0-0xFF\n"
"        if (!(c >= 0xA0 && c <= 0xFF)) latin1 = 0;\n"
"        // Win1251 check: 0xC0-0xFF, 0x80-0xBF\n"
"        if (!((c >= 0xC0 && c <= 0xFF) || (c >= 0x80 && c <= 0xBF))) win1251 = 0;\n"
"        i++;\n"
"    }\n"
"    if (utf8) encoding = 0;\n"
"    else if (win1251) encoding = 1;\n"
"    else if (latin1) encoding = 2;\n"
"    else if (ascii) encoding = 3;\n"
"    out_encodings[idx] = encoding;\n"
"}\n";

VTL_AppResult VTL_sub_OpenclDetectEncoding(const char** in_texts, int** out_encodings, size_t count) {
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;

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

    int* encodings = (int*)malloc(count * sizeof(int));

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
    kernel = clCreateKernel(program, "detect_encoding", &err);
    if (!kernel || err != CL_SUCCESS) { clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); goto cleanup; }

    cl_mem buf_in_data = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, total_in, in_data, &err);
    cl_mem buf_offsets = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, count * sizeof(int), in_offsets, &err);
    cl_mem buf_lengths = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, count * sizeof(int), in_lengths, &err);
    cl_mem buf_out_encodings = clCreateBuffer(context, CL_MEM_WRITE_ONLY, count * sizeof(int), NULL, &err);

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_in_data);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_offsets);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &buf_lengths);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), &buf_out_encodings);

    size_t global = count;
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) goto cleanup_cl;
    clFinish(queue);

    err = clEnqueueReadBuffer(queue, buf_out_encodings, CL_TRUE, 0, count * sizeof(int), encodings, 0, NULL, NULL);
    if (err != CL_SUCCESS) goto cleanup_cl;

    *out_encodings = encodings;

cleanup_cl:
    clReleaseMemObject(buf_in_data);
    clReleaseMemObject(buf_offsets);
    clReleaseMemObject(buf_lengths);
    clReleaseMemObject(buf_out_encodings);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
cleanup:
    free(in_offsets); free(in_lengths); free(in_data);
    return VTL_res_kOk;
} 