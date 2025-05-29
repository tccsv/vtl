#include <VTL/media_container/sub/opencl/VTL_sub_opencl_format_time.h>
#include <OpenCL/opencl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VTL_SUB_OPENCL_KERNEL_SOURCE_FORMAT_TIME
"__kernel void format_time(__global const double* in_times, __global char* out_data, __global int* out_offsets, int format) {\n"
"    int idx = get_global_id(0);\n"
"    double t = in_times[idx];\n"
"    int hours = (int)(t / 3600.0);\n"
"    int minutes = (int)((t - hours * 3600) / 60.0);\n"
"    int seconds = (int)(t) % 60;\n"
"    int ms = (int)((t - (int)t) * 1000.0 + 0.5);\n"
"    int cs = (int)((t - (int)t) * 100.0 + 0.5);\n"
"    int offset = out_offsets[idx];\n"
"
"    if (format == 0) { // SRT 00:00:00,000\n"
"        sprintf(out_data + offset, \"%02d:%02d:%02d,%03d\", hours, minutes, seconds, ms);\n"
"    } else if (format == 1) { // ASS 0:00:00.00\n"
"        sprintf(out_data + offset, \"%d:%02d:%02d.%02d\", hours, minutes, seconds, cs);\n"
"    } else { // VTT 00:00:00.000\n"
"        sprintf(out_data + offset, \"%02d:%02d:%02d.%03d\", hours, minutes, seconds, ms);\n"
"    }\n"
"}\n";

VTL_AppResult VTL_sub_OpenclFormatTime(const double* in_times, char*** out_texts, size_t count, int format) {
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
    program = clCreateProgramWithSource(context, 1, &VTL_SUB_OPENCL_KERNEL_SOURCE_FORMAT_TIME, NULL, &err);
    if (!program || err != CL_SUCCESS) { clReleaseCommandQueue(queue); clReleaseContext(context); return VTL_res_opencl_kProgramError; }
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) { clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); return VTL_res_opencl_kBuildError; }
    kernel = clCreateKernel(program, "format_time", &err);
    if (!kernel || err != CL_SUCCESS) { clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); return VTL_res_opencl_kKernelError; }

    // Максимальная длина строки: SRT/VTT = 12, ASS = 11 (+1 нуль-терминатор)
    int max_len = 16;
    int* out_offsets = (int*)malloc(count * sizeof(int));
    for (size_t i = 0; i < count; ++i) out_offsets[i] = (int)(i * max_len);
    char* out_data = (char*)malloc(count * max_len);

    cl_mem buf_in_times = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, count * sizeof(double), (void*)in_times, &err);
    cl_mem buf_out_data = clCreateBuffer(context, CL_MEM_WRITE_ONLY, count * max_len, NULL, &err);
    cl_mem buf_out_offsets = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, count * sizeof(int), out_offsets, &err);

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_in_times);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_out_data);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &buf_out_offsets);
    clSetKernelArg(kernel, 3, sizeof(int), &format);

    size_t global = count;
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        clReleaseMemObject(buf_in_times);
        clReleaseMemObject(buf_out_data);
        clReleaseMemObject(buf_out_offsets);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        free(out_offsets); free(out_data);
        return VTL_res_opencl_kLaunchError;
    }
    clFinish(queue);

    char** texts = (char**)malloc(count * sizeof(char*));
    err = clEnqueueReadBuffer(queue, buf_out_data, CL_TRUE, 0, count * max_len, out_data, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        clReleaseMemObject(buf_in_times);
        clReleaseMemObject(buf_out_data);
        clReleaseMemObject(buf_out_offsets);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        free(out_offsets); free(out_data); free(texts);
        return VTL_res_opencl_kReadBufferError;
    }
    for (size_t i = 0; i < count; ++i) {
        texts[i] = (char*)malloc(max_len);
        memcpy(texts[i], out_data + i * max_len, max_len);
        texts[i][max_len-1] = '\0';
    }
    *out_texts = texts;

    clReleaseMemObject(buf_in_times);
    clReleaseMemObject(buf_out_data);
    clReleaseMemObject(buf_out_offsets);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    free(out_offsets); free(out_data);
    return VTL_res_kOk;
} 