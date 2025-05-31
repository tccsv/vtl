#include "VTL_converter.h"
#include "VTL_file_ops.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    char* content;
    size_t size;
    VTL_MarkupType type;
} VTL_InternalContent;

static void VTL_free_internal_content(VTL_InternalContent* content) {
    if (content && content->content) {
        free(content->content);
        content->content = NULL;
        content->size = 0;
    }
}

static VTL_AppResult VTL_convert_std_md_to_tg_md_internal(const char* std_md, size_t size, char** tg_md, size_t* tg_md_size) {
    *tg_md = (char*)malloc(size + 1);
    if (!*tg_md) {
        return VTL_res_kError;
    }

    size_t out_pos = 0;
    for (size_t i = 0; i < size; i++) {
        if (i + 1 < size && std_md[i] == '*' && std_md[i + 1] == '*') {
            (*tg_md)[out_pos++] = '*';
            i++;
            continue;
        }
        if (std_md[i] == '_') {
            (*tg_md)[out_pos++] = '_';
            continue;
        }
        if (i + 2 < size && std_md[i] == '`' && std_md[i + 1] == '`' && std_md[i + 2] == '`') {
            (*tg_md)[out_pos++] = '`';
            (*tg_md)[out_pos++] = '`';
            (*tg_md)[out_pos++] = '`';
            i += 2;
            continue;
        }
        (*tg_md)[out_pos++] = std_md[i];
    }
    (*tg_md)[out_pos] = '\0';
    *tg_md_size = out_pos;

    return VTL_res_kOk;
}

static VTL_AppResult VTL_convert_tg_md_to_std_md_internal(const char* tg_md, size_t size, char** std_md, size_t* std_md_size) {
    *std_md = (char*)malloc(size + 1);
    if (!*std_md) {
        return VTL_res_kError;
    }

    size_t out_pos = 0;
    for (size_t i = 0; i < size; i++) {
        if (tg_md[i] == '*') {
            (*std_md)[out_pos++] = '*';
            (*std_md)[out_pos++] = '*';
            continue;
        }
        if (tg_md[i] == '_') {
            (*std_md)[out_pos++] = '_';
            continue;
        }
        if (i + 2 < size && tg_md[i] == '`' && tg_md[i + 1] == '`' && tg_md[i + 2] == '`') {
            (*std_md)[out_pos++] = '`';
            (*std_md)[out_pos++] = '`';
            (*std_md)[out_pos++] = '`';
            i += 2;
            continue;
        }
        (*std_md)[out_pos++] = tg_md[i];
    }
    (*std_md)[out_pos] = '\0';
    *std_md_size = out_pos;

    return VTL_res_kOk;
}

static VTL_AppResult VTL_convert_md_to_plain_internal(const char* md, size_t size, char** plain, size_t* plain_size) {
    *plain = (char*)malloc(size + 1);
    if (!*plain) {
        return VTL_res_kError;
    }

    size_t out_pos = 0;
    for (size_t i = 0; i < size; i++) {
        if (md[i] == '*' || md[i] == '_' || md[i] == '`' || md[i] == '#') {
            continue;
        }
        (*plain)[out_pos++] = md[i];
    }
    (*plain)[out_pos] = '\0';
    *plain_size = out_pos;

    return VTL_res_kOk;
}

VTL_AppResult VTL_convert_markup_s(const VTL_ConversionConfig* config) {
    if (!config || !config->input_file || !config->output_file) {
        return VTL_res_kError;
    }

    char* input_content;
    size_t input_size;
    VTL_FileResult file_result = VTL_file_read_s(config->input_file, &input_content, &input_size);
    if (file_result != VTL_FILE_RES_OK) {
        return VTL_res_kError;
    }

    char* output_content;
    size_t output_size;
    VTL_AppResult convert_result;

    switch (config->input_type) {
        case VTL_MARKUP_TYPE_STANDARD_MD:
            if (config->output_type == VTL_MARKUP_TYPE_TELEGRAM_MD) {
                convert_result = VTL_convert_std_md_to_tg_md_internal(input_content, input_size, &output_content, &output_size);
            } else if (config->output_type == VTL_MARKUP_TYPE_PLAIN_TEXT) {
                convert_result = VTL_convert_md_to_plain_internal(input_content, input_size, &output_content, &output_size);
            } else {
                convert_result = VTL_res_kError;
            }
            break;

        case VTL_MARKUP_TYPE_TELEGRAM_MD:
            if (config->output_type == VTL_MARKUP_TYPE_STANDARD_MD) {
                convert_result = VTL_convert_tg_md_to_std_md_internal(input_content, input_size, &output_content, &output_size);
            } else if (config->output_type == VTL_MARKUP_TYPE_PLAIN_TEXT) {
                convert_result = VTL_convert_md_to_plain_internal(input_content, input_size, &output_content, &output_size);
            } else {
                convert_result = VTL_res_kError;
            }
            break;

        default:
            convert_result = VTL_res_kError;
            break;
    }

    free(input_content);

    if (convert_result != VTL_res_kOk) {
        return convert_result;
    }

    file_result = VTL_file_write_s(config->output_file, output_content, output_size);
    free(output_content);

    return (file_result == VTL_FILE_RES_OK) ? VTL_res_kOk : VTL_res_kError;
}

VTL_AppResult VTL_std_md_to_tg_md_s(const char* std_md, size_t std_md_size, char** tg_md, size_t* tg_md_size) {
    return VTL_convert_std_md_to_tg_md_internal(std_md, std_md_size, tg_md, tg_md_size);
}

VTL_AppResult VTL_tg_md_to_std_md_s(const char* tg_md, size_t tg_md_size, char** std_md, size_t* std_md_size) {
    return VTL_convert_tg_md_to_std_md_internal(tg_md, tg_md_size, std_md, std_md_size);
}

VTL_AppResult VTL_md_to_plain_s(const char* md, size_t md_size, char** plain, size_t* plain_size) {
    return VTL_convert_md_to_plain_internal(md, md_size, plain, plain_size);
} 