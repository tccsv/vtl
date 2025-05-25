#include "VTL_converter.h"
#include "VTL_file_ops.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    char* content;
    size_t size;
    VTL_publication_marked_text_MarkupType type;
} VTL_publication_InternalContent;

static void VTL_publication_free_internal_content(VTL_publication_InternalContent* content) {
    if (content && content->content) {
        free(content->content);
        content->content = NULL;
        content->size = 0;
    }
}

static VTL_publication_app_result VTL_publication_convert_std_md_to_tg_md_internal(const char* std_md, size_t size, char** tg_md, size_t* tg_md_size) {
    *tg_md = (char*)malloc(size + 1);
    if (!*tg_md) {
        return VTL_publication_res_kError;
    }

    size_t out_pos = 0;
    for (size_t i = 0; i < size; i++) {
        if (i + 1 < size && std_md[i] == '*' && std_md[i + 1] == '*') {
            (*tg_md)[out_pos++] = '*';
            i++;
        } else if (std_md[i] == '_') {
            (*tg_md)[out_pos++] = '_';
        } else if (i + 2 < size && std_md[i] == '`' && std_md[i + 1] == '`' && std_md[i + 2] == '`') {
            (*tg_md)[out_pos++] = '`';
            (*tg_md)[out_pos++] = '`';
            (*tg_md)[out_pos++] = '`';
            i += 2;
        } else {
            (*tg_md)[out_pos++] = std_md[i];
        }
    }
    (*tg_md)[out_pos] = '\0';
    *tg_md_size = out_pos;

    return VTL_publication_res_kOk;
}

static VTL_publication_app_result VTL_publication_convert_tg_md_to_std_md_internal(const char* tg_md, size_t size, char** std_md, size_t* std_md_size) {
    *std_md = (char*)malloc(size + 1);
    if (!*std_md) {
        return VTL_publication_res_kError;
    }

    size_t out_pos = 0;
    for (size_t i = 0; i < size; i++) {
        if (tg_md[i] == '*') {
            (*std_md)[out_pos++] = '*';
            (*std_md)[out_pos++] = '*';
        } else if (tg_md[i] == '_') {
            (*std_md)[out_pos++] = '_';
        } else if (i + 2 < size && tg_md[i] == '`' && tg_md[i + 1] == '`' && tg_md[i + 2] == '`') {
            (*std_md)[out_pos++] = '`';
            (*std_md)[out_pos++] = '`';
            (*std_md)[out_pos++] = '`';
            i += 2;
        } else {
            (*std_md)[out_pos++] = tg_md[i];
        }
    }
    (*std_md)[out_pos] = '\0';
    *std_md_size = out_pos;

    return VTL_publication_res_kOk;
}

static VTL_publication_app_result VTL_publication_convert_md_to_plain_internal(const char* md, size_t size, char** plain, size_t* plain_size) {
    *plain = (char*)malloc(size + 1);
    if (!*plain) {
        return VTL_publication_res_kError;
    }

    size_t out_pos = 0;
    for (size_t i = 0; i < size; i++) {
        if (md[i] != '*' && md[i] != '_' && md[i] != '`' && md[i] != '#') {
            (*plain)[out_pos++] = md[i];
        }
    }
    (*plain)[out_pos] = '\0';
    *plain_size = out_pos;

    return VTL_publication_res_kOk;
}

VTL_publication_app_result VTL_publication_convert_markup_s(const VTL_publication_ConversionConfig* config) {
    if (!config || !config->input_file || !config->output_file) {
        return VTL_publication_res_kError;
    }

    char* input_content;
    size_t input_size;
    VTL_publication_file_result file_result = VTL_publication_file_read_s(config->input_file, &input_content, &input_size);
    if (file_result != VTL_publication_file_res_kOk) {
        return VTL_publication_res_kError;
    }

    char* output_content;
    size_t output_size;
    VTL_publication_app_result convert_result;

    switch (config->input_type) {
        case VTL_publication_markup_type_kStandardMD:
            if (config->output_type == VTL_publication_markup_type_kTelegramMD) {
                convert_result = VTL_publication_convert_std_md_to_tg_md_internal(input_content, input_size, &output_content, &output_size);
            } else if (config->output_type == VTL_publication_markup_type_kPlainText) {
                convert_result = VTL_publication_convert_md_to_plain_internal(input_content, input_size, &output_content, &output_size);
            } else {
                convert_result = VTL_publication_res_kError;
            }
            break;

        case VTL_publication_markup_type_kTelegramMD:
            if (config->output_type == VTL_publication_markup_type_kStandardMD) {
                convert_result = VTL_publication_convert_tg_md_to_std_md_internal(input_content, input_size, &output_content, &output_size);
            } else if (config->output_type == VTL_publication_markup_type_kPlainText) {
                convert_result = VTL_publication_convert_md_to_plain_internal(input_content, input_size, &output_content, &output_size);
            } else {
                convert_result = VTL_publication_res_kError;
            }
            break;

        default:
            convert_result = VTL_publication_res_kError;
            break;
    }

    free(input_content);

    if (convert_result != VTL_publication_res_kOk) {
        return convert_result;
    }

    file_result = VTL_publication_file_write_s(config->output_file, output_content, output_size);
    free(output_content);

    return (file_result == VTL_publication_file_res_kOk) ? VTL_publication_res_kOk : VTL_publication_res_kError;
}

VTL_publication_app_result VTL_publication_std_md_to_tg_md_s(const char* std_md, size_t std_md_size, char** tg_md, size_t* tg_md_size) {
    return VTL_publication_convert_std_md_to_tg_md_internal(std_md, std_md_size, tg_md, tg_md_size);
}

VTL_publication_app_result VTL_publication_tg_md_to_std_md_s(const char* tg_md, size_t tg_md_size, char** std_md, size_t* std_md_size) {
    return VTL_publication_convert_tg_md_to_std_md_internal(tg_md, tg_md_size, std_md, std_md_size);
}

VTL_publication_app_result VTL_publication_md_to_plain_s(const char* md, size_t md_size, char** plain, size_t* plain_size) {
    return VTL_publication_convert_md_to_plain_internal(md, md_size, plain, plain_size);
} 