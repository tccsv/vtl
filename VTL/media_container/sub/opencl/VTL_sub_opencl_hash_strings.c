#include <VTL/media_container/sub/opencl/VTL_sub_opencl_hash_strings.h>
#include <OpenCL/opencl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// OpenCL-ядро для вычисления CRC32 для каждой строки
const char* kernelSource =
"__kernel void hash_strings(__global const char* in_data, __global int* offsets, __global int* lengths, __global uint* out_hashes) {\n"
"    int idx = get_global_id(0);\n"
"    int in_offset = offsets[idx];\n"
"    int in_len = lengths[idx];\n"
"    uint crc = 0xFFFFFFFFU;\n"
"    for (int k = 0; k < in_len; ++k) {\n"
"        unsigned char c = in_data[in_offset + k];\n"
"        crc ^= c;\n"
"        for (int j = 0; j < 8; ++j) {\n"
"            uint mask = -(crc & 1U);\n"
"            crc = (crc >> 1) ^ (0xEDB88320U & mask);\n"
"        }\n"
"    }\n"
"    out_hashes[idx] = ~crc;\n"
"}\n";

VTL_AppResult VTL_sub_OpenclHashStrings(const char** in_texts, uint32_t** out_hashes, size_t count) {
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
    program = clCreateProgramWithSource(context, 1, &kernelSource, NULL, &err);
    if (!program || err != CL_SUCCESS) { clReleaseCommandQueue(queue); clReleaseContext(context); return VTL_res_opencl_kProgramError; }
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) { clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); return VTL_res_opencl_kBuildError; }
    kernel = clCreateKernel(program, "hash_strings", &err);
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
    cl_mem buf_out_hashes = clCreateBuffer(context, CL_MEM_WRITE_ONLY, count * sizeof(uint32_t), NULL, &err);

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_in_data);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_offsets);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &buf_lengths);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), &buf_out_hashes);

    size_t global = count;
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        clReleaseMemObject(buf_in_data);
        clReleaseMemObject(buf_offsets);
        clReleaseMemObject(buf_lengths);
        clReleaseMemObject(buf_out_hashes);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        free(in_offsets); free(in_lengths); free(in_data);
        return VTL_res_opencl_kLaunchError;
    }
    clFinish(queue);
    uint32_t* hashes = (uint32_t*)malloc(count * sizeof(uint32_t));
    err = clEnqueueReadBuffer(queue, buf_out_hashes, CL_TRUE, 0, count * sizeof(uint32_t), hashes, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        clReleaseMemObject(buf_in_data);
        clReleaseMemObject(buf_offsets);
        clReleaseMemObject(buf_lengths);
        clReleaseMemObject(buf_out_hashes);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        free(in_offsets); free(in_lengths); free(in_data); free(hashes);
        return VTL_res_opencl_kReadBufferError;
    }
    *out_hashes = hashes;

    clReleaseMemObject(buf_in_data);
    clReleaseMemObject(buf_offsets);
    clReleaseMemObject(buf_lengths);
    clReleaseMemObject(buf_out_hashes);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    free(in_offsets); free(in_lengths); free(in_data);
    return VTL_res_kOk;
} 