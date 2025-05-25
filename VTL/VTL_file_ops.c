#include "VTL_file_ops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

VTL_publication_file_result VTL_publication_file_read_s(const char* filename, char** content, size_t* size) {
    if (!filename || !content || !size) {
        return VTL_publication_file_res_kErrorMemory;
    }

    FILE* file = fopen(filename, "rb");
    if (!file) {
        return VTL_publication_file_res_kErrorOpen;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory
    *content = (char*)malloc(*size + 1);
    if (!*content) {
        fclose(file);
        return VTL_publication_file_res_kErrorMemory;
    }

    // Read file
    size_t read_size = fread(*content, 1, *size, file);
    if (read_size != *size) {
        free(*content);
        fclose(file);
        return VTL_publication_file_res_kErrorRead;
    }

    (*content)[*size] = '\0';
    fclose(file);
    return VTL_publication_file_res_kOk;
}

VTL_publication_file_result VTL_publication_file_write_s(const char* filename, const char* content, size_t size) {
    if (!filename || !content) {
        return VTL_publication_file_res_kErrorMemory;
    }

    FILE* file = fopen(filename, "wb");
    if (!file) {
        return VTL_publication_file_res_kErrorOpen;
    }

    size_t written = fwrite(content, 1, size, file);
    fclose(file);

    if (written != size) {
        return VTL_publication_file_res_kErrorWrite;
    }

    return VTL_publication_file_res_kOk;
}

VTL_publication_file_result VTL_publication_file_append_s(const char* filename, const char* content, size_t size) {
    if (!filename || !content) {
        return VTL_publication_file_res_kErrorMemory;
    }

    FILE* file = fopen(filename, "ab");
    if (!file) {
        return VTL_publication_file_res_kErrorOpen;
    }

    size_t written = fwrite(content, 1, size, file);
    fclose(file);

    if (written != size) {
        return VTL_publication_file_res_kErrorWrite;
    }

    return VTL_publication_file_res_kOk;
}

const char* VTL_publication_file_get_error_message(VTL_publication_file_result result) {
    switch (result) {
        case VTL_publication_file_res_kOk:
            return "Operation completed successfully";
        case VTL_publication_file_res_kErrorOpen:
            return "Failed to open file";
        case VTL_publication_file_res_kErrorRead:
            return "Failed to read file";
        case VTL_publication_file_res_kErrorWrite:
            return "Failed to write file";
        case VTL_publication_file_res_kErrorClose:
            return "Failed to close file";
        case VTL_publication_file_res_kErrorMemory:
            return "Memory allocation failed";
        default:
            return "Unknown error";
    }
} 