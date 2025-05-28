#include <VTL/media_container/sub/opencl/VTL_sub_opencl_apply_ass_style.h>
#include <OpenCL/opencl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// OpenCL-ядро для применения/удаления ASS-стиля
const char* kernelSource =
"__kernel void apply_ass_style(__global const char* in_data, __global int* offsets, __global int* lengths, __global char* out_data, __global int* out_offsets, __global const char* style_tag, int tag_len, int mode) {\n"
"    int idx = get_global_id(0);\n"
"    int in_offset = offsets[idx];\n"
"    int in_len = lengths[idx];\n"
"    int out_offset = out_offsets[idx];\n"
"    if (mode == 1) { // добавить стиль\n"
"        // Копируем style_tag в начало\n"
"        for (int i = 0; i < tag_len; ++i) out_data[out_offset + i] = style_tag[i];\n"
"        // Копируем строку\n"
"        for (int i = 0; i < in_len; ++i) out_data[out_offset + tag_len + i] = in_data[in_offset + i];\n"
"        // Копируем style_tag в конец\n"
"        for (int i = 0; i < tag_len; ++i) out_data[out_offset + tag_len + in_len + i] = style_tag[i];\n"
"        out_data[out_offset + tag_len + in_len + tag_len] = '\0';\n"
"    } else { // удалить стиль\n"
"        int j = 0, k = 0;\n"
"        while (k < in_len) {\n"
"            int match = 1;\n"
"            for (int t = 0; t < tag_len && k + t < in_len; ++t) {\n"
"                if (in_data[in_offset + k + t] != style_tag[t]) { match = 0; break; }\n"
"            }\n"
"            if (match && tag_len > 0 && k + tag_len <= in_len) { k += tag_len; }\n"
"            else out_data[out_offset + j++] = in_data[in_offset + k++];\n"
"        }\n"
"        out_data[out_offset + j] = '\0';\n"
"    }\n"
"}\n";

VTL_AppResult VTL_sub_OpenclApplyAssStyle(const char** in_texts, char*** out_texts, size_t count, const char* style_tag, int mode) {
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;

    size_t tag_len = strlen(style_tag);
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
    for (size_t i = 0; i < count; ++i) out_lengths[i] = in_lengths[i] + (mode == 1 ? 2 * tag_len : 0) + 1;
    size_t* out_offsets = (size_t*)malloc(count * sizeof(size_t));
    size_t total_out = 0;
    for (size_t i = 0; i < count; ++i) {
        out_offsets[i] = total_out;
        total_out += out_lengths[i];
    }
    char* out_data = (char*)malloc(total_out);

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
    kernel = clCreateKernel(program, "apply_ass_style", &err);
    if (!kernel || err != CL_SUCCESS) { clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); return VTL_res_opencl_kKernelError; }

    cl_mem buf_in_data = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, total_in, in_data, &err);
    cl_mem buf_offsets = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, count * sizeof(int), in_offsets, &err);
    cl_mem buf_lengths = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, count * sizeof(int), in_lengths, &err);
    cl_mem buf_out_data = clCreateBuffer(context, CL_MEM_WRITE_ONLY, total_out, NULL, &err);
    cl_mem buf_out_offsets = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, count * sizeof(int), out_offsets, &err);
    cl_mem buf_style_tag = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, tag_len, style_tag, &err);

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_in_data);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_offsets);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &buf_lengths);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), &buf_out_data);
    clSetKernelArg(kernel, 4, sizeof(cl_mem), &buf_out_offsets);
    clSetKernelArg(kernel, 5, sizeof(cl_mem), &buf_style_tag);
    clSetKernelArg(kernel, 6, sizeof(int), &tag_len);
    clSetKernelArg(kernel, 7, sizeof(int), &mode);

    size_t global = count;
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        clReleaseMemObject(buf_in_data);
        clReleaseMemObject(buf_offsets);
        clReleaseMemObject(buf_lengths);
        clReleaseMemObject(buf_out_data);
        clReleaseMemObject(buf_out_offsets);
        clReleaseMemObject(buf_style_tag);
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
        clReleaseMemObject(buf_style_tag);
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
    clReleaseMemObject(buf_style_tag);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    free(in_offsets); free(in_lengths); free(in_data); free(out_lengths); free(out_offsets); free(out_data);
    return VTL_res_kOk;
} 