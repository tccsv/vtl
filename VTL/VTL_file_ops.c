#include "VTL_file_ops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// самые разные файлвоые операции
VTL_FileResult VTL_file_read_s(const char* filename, char** content, size_t* size) {
    if (!filename || !content || !size) {
        return VTL_FILE_RES_ERROR_MEMORY;
    }

    FILE* file = fopen(filename, "rb");
    if (!file) {
        return VTL_FILE_RES_ERROR_OPEN;
    }

    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);

    *content = (char*)malloc(*size + 1);
    if (!*content) {
        fclose(file);
        return VTL_FILE_RES_ERROR_MEMORY;
    }

    size_t read_size = fread(*content, 1, *size, file);
    if (read_size != *size) {
        free(*content);
        fclose(file);
        return VTL_FILE_RES_ERROR_READ;
    }

    (*content)[*size] = '\0';
    fclose(file);
    return VTL_FILE_RES_OK;
}

VTL_FileResult VTL_file_write_s(const char* filename, const char* content, size_t size) {
    if (!filename || !content) {
        return VTL_FILE_RES_ERROR_MEMORY;
    }

    FILE* file = fopen(filename, "wb");
    if (!file) {
        return VTL_FILE_RES_ERROR_OPEN;
    }

    size_t written = fwrite(content, 1, size, file);
    fclose(file);

    if (written != size) {
        return VTL_FILE_RES_ERROR_WRITE;
    }

    return VTL_FILE_RES_OK;
}

VTL_FileResult VTL_file_append_s(const char* filename, const char* content, size_t size) {
    if (!filename || !content) {
        return VTL_FILE_RES_ERROR_MEMORY;
    }

    FILE* file = fopen(filename, "ab");
    if (!file) {
        return VTL_FILE_RES_ERROR_OPEN;
    }

    size_t written = fwrite(content, 1, size, file);
    fclose(file);

    if (written != size) {
        return VTL_FILE_RES_ERROR_WRITE;
    }

    return VTL_FILE_RES_OK;
}

const char* VTL_file_get_error_message(VTL_FileResult result) {
    switch (result) {
        case VTL_FILE_RES_OK:
            return "Operation completed successfully";
        case VTL_FILE_RES_ERROR_OPEN:
            return "Failed to open file";
        case VTL_FILE_RES_ERROR_READ:
            return "Failed to read file";
        case VTL_FILE_RES_ERROR_WRITE:
            return "Failed to write file";
        case VTL_FILE_RES_ERROR_CLOSE:
            return "Failed to close file";
        case VTL_FILE_RES_ERROR_MEMORY:
            return "Memory allocation failed";
        default:
            return "Unknown error";
    }
} 