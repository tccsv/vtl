#include <VTL/VTL_file_ops.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

VTL_publication_file_result VTL_publication_FileReadS(const char* filename, char** content, size_t* size) {
    if (!content || !size) {
        return VTL_publication_file_res_kErrorMemory;
    }

    FILE* file = fopen(filename, "rb");
    if (!file) {
        return VTL_publication_file_res_kErrorOpen;
    }

    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);

    *content = (char*)malloc(*size + 1);
    if (!*content) {
        fclose(file);
        return VTL_publication_file_res_kErrorMemory;
    }

    if (fread(*content, 1, *size, file) != *size) {
        free(*content);
        fclose(file);
        return VTL_publication_file_res_kErrorRead;
    }

    (*content)[*size] = '\0';
    fclose(file);
    return VTL_publication_file_res_kOk;
}

VTL_publication_file_result VTL_publication_FileWriteS(const char* filename, const char* content, size_t size) {
    if (!content) {
        return VTL_publication_file_res_kErrorMemory;
    }

    FILE* file = fopen(filename, "wb");
    if (!file) {
        return VTL_publication_file_res_kErrorOpen;
    }

    if (fwrite(content, 1, size, file) != size) {
        fclose(file);
        return VTL_publication_file_res_kErrorWrite;
    }

    fclose(file);
    return VTL_publication_file_res_kOk;
}

VTL_publication_file_result VTL_publication_FileAppendS(const char* filename, const char* content, size_t size) {
    if (!content) {
        return VTL_publication_file_res_kErrorMemory;
    }

    FILE* file = fopen(filename, "ab");
    if (!file) {
        return VTL_publication_file_res_kErrorOpen;
    }

    if (fwrite(content, 1, size, file) != size) {
        fclose(file);
        return VTL_publication_file_res_kErrorWrite;
    }

    fclose(file);
    return VTL_publication_file_res_kOk;
}

const char* VTL_publication_FileGetErrorMessage(VTL_publication_file_result result) {
    switch (result) {
        case VTL_publication_file_res_kOk:
            return "No error";
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