#include <VTL/VTL_converter.h>
#include <VTL/VTL_file_ops.h>
#include <stdlib.h>
#include <string.h>

typedef struct _VTL_publication_InternalContent
{
    char* data;
    size_t size;
    VTL_publication_marked_text_MarkupType type;
} VTL_publication_InternalContent;

static void VTL_publication_FreeInternalContent(VTL_publication_InternalContent* content) {
    if (content) {
        if (content->data) {
            free(content->data);
            content->data = NULL;
        }
        content->size = 0;
    }
}

static VTL_publication_app_result VTL_publication_ConvertStdMdToTgMdInternal(const char* std_md, size_t size, char** tg_md, size_t* tg_md_size) {
    if (!std_md || !tg_md || !tg_md_size) {
        return VTL_publication_res_kError;
    }

    
    *tg_md = (char*)malloc(size * 2 + 1);
    if (!*tg_md) {
        return VTL_publication_res_kError;
    }

    size_t out_pos = 0;
    for (size_t i = 0; i < size; i++) {
        // Обработка жирного текста (**текст** -> *текст*)
        if (i + 1 < size && std_md[i] == '*' && std_md[i + 1] == '*') {
            (*tg_md)[out_pos++] = '*';
            i++; // Пропускаем второй *
        }
        // Обработка курсива (_текст_ -> _текст_)
        else if (std_md[i] == '_') {
            (*tg_md)[out_pos++] = '_';
        }
        // Обработка блоков кода (```код``` -> ```код```)
        else if (i + 2 < size && std_md[i] == '`' && std_md[i + 1] == '`' && std_md[i + 2] == '`') {
            (*tg_md)[out_pos++] = '`';
            (*tg_md)[out_pos++] = '`';
            (*tg_md)[out_pos++] = '`';
            i += 2;
        }
        
        else if (std_md[i] == '`') {
            (*tg_md)[out_pos++] = '`';
        }
        
        else if (std_md[i] == '#' && (i == 0 || std_md[i - 1] == '\n')) {
            size_t header_end = i;
            while (header_end < size && std_md[header_end] != '\n') {
                header_end++;
            }
            if (header_end > i + 1) {
                (*tg_md)[out_pos++] = '*';
                i++; // Пропускаем #
                while (i < header_end && std_md[i] == ' ') i++; 
                while (i < header_end) {
                    (*tg_md)[out_pos++] = std_md[i++];
                }
                (*tg_md)[out_pos++] = '*';
                i--; // Корректируем инкремент цикла
            }
        }
        // Обработка ссылок ([текст](url) -> текст)
        else if (std_md[i] == '[') {
            size_t link_end = i;
            while (link_end < size && std_md[link_end] != ']') {
                link_end++;
            }
            if (link_end < size && link_end + 1 < size && std_md[link_end + 1] == '(') {
                i++; // Пропускаем [
                while (i < link_end) {
                    (*tg_md)[out_pos++] = std_md[i++];
                }
                i = link_end + 1;
                while (i < size && std_md[i] != ')') i++;
            } else {
                (*tg_md)[out_pos++] = std_md[i];
            }
        }
        // Копируем остальные символы как есть
        else {
            (*tg_md)[out_pos++] = std_md[i];
        }
    }
    (*tg_md)[out_pos] = '\0';
    *tg_md_size = out_pos;

    // Перевыделяем память под реальный размер
    char* temp = (char*)realloc(*tg_md, out_pos + 1);
    if (temp) {
        *tg_md = temp;
    }

    return VTL_publication_res_kOk;
}

static VTL_publication_app_result VTL_publication_ConvertTgMdToStdMdInternal(const char* tg_md, size_t size, char** std_md, size_t* std_md_size) {
    if (!tg_md || !std_md || !std_md_size) {
        return VTL_publication_res_kError;
    }

    // Выделяем максимально возможный размер (худший случай: каждый символ форматирования нужно удвоить)
    *std_md = (char*)malloc(size * 2 + 1);
    if (!*std_md) {
        return VTL_publication_res_kError;
    }

    size_t out_pos = 0;
    for (size_t i = 0; i < size; i++) {
        // Обработка жирного текста (*текст* -> **текст**)
        if (tg_md[i] == '*') {
            (*std_md)[out_pos++] = '*';
            (*std_md)[out_pos++] = '*';
        }
        // Обработка курсива (_текст_ -> _текст_)
        else if (tg_md[i] == '_') {
            (*std_md)[out_pos++] = '_';
        }
        // Обработка блоков кода (```код``` -> ```код```)
        else if (i + 2 < size && tg_md[i] == '`' && tg_md[i + 1] == '`' && tg_md[i + 2] == '`') {
            (*std_md)[out_pos++] = '`';
            (*std_md)[out_pos++] = '`';
            (*std_md)[out_pos++] = '`';
            i += 2;
        }
        // Обработка встроенного кода (`код` -> `код`)
        else if (tg_md[i] == '`') {
            (*std_md)[out_pos++] = '`';
        }
        // Копируем остальные символы как есть
        else {
            (*std_md)[out_pos++] = tg_md[i];
        }
    }
    (*std_md)[out_pos] = '\0';
    *std_md_size = out_pos;

    char* temp = (char*)realloc(*std_md, out_pos + 1);
    if (temp) {
        *std_md = temp;
    }

    return VTL_publication_res_kOk;
}

static VTL_publication_app_result VTL_publication_ConvertMdToPlainInternal(const char* md, size_t size, char** plain, size_t* plain_size) {
    if (!md || !plain || !plain_size) {
        return VTL_publication_res_kError;
    }

    // Выделяем память того же размера (обычный текст не может быть длиннее)
    *plain = (char*)malloc(size + 1);
    if (!*plain) {
        return VTL_publication_res_kError;
    }

    size_t out_pos = 0;
    for (size_t i = 0; i < size; i++) {
        if (md[i] == '*' || md[i] == '_' || md[i] == '`' || md[i] == '#') {
            // Пропускаем символы форматирования
        }
        else if (md[i] == '(') {
            // Пропускаем URL в ссылках
            while (i < size && md[i] != ')') {
                i++;
            }
        }
        else {
            // Копируем остальные символы
            (*plain)[out_pos++] = md[i];
        }
    }
    (*plain)[out_pos] = '\0';
    *plain_size = out_pos;

    // Перевыделяем память под реальный размер
    char* temp = (char*)realloc(*plain, out_pos + 1);
    if (temp) {
        *plain = temp;
    }

    return VTL_publication_res_kOk;
}

VTL_publication_app_result VTL_publication_ConvertMarkupS(const VTL_publication_ConversionConfig* config) {
    if (!config || !config->input_file || !config->output_file) {
        return VTL_publication_res_kError;
    }

    char* input_content = NULL;
    size_t input_size = 0;
    VTL_publication_file_result file_result = VTL_publication_FileReadS(config->input_file, &input_content, &input_size);
    if (file_result != VTL_publication_file_res_kOk) {
        return VTL_publication_res_kError;
    }

    char* output_content = NULL;
    size_t output_size = 0;
    VTL_publication_app_result convert_result;

    switch (config->input_type) {
        case VTL_publication_markup_type_kStandardMD:
            if (config->output_type == VTL_publication_markup_type_kTelegramMD) {
                convert_result = VTL_publication_ConvertStdMdToTgMdInternal(input_content, input_size, &output_content, &output_size);
            } else if (config->output_type == VTL_publication_markup_type_kPlainText) {
                convert_result = VTL_publication_ConvertMdToPlainInternal(input_content, input_size, &output_content, &output_size);
            } else {
                convert_result = VTL_publication_res_kError;
            }
            break;

        case VTL_publication_markup_type_kTelegramMD:
            if (config->output_type == VTL_publication_markup_type_kStandardMD) {
                convert_result = VTL_publication_ConvertTgMdToStdMdInternal(input_content, input_size, &output_content, &output_size);
            } else if (config->output_type == VTL_publication_markup_type_kPlainText) {
                convert_result = VTL_publication_ConvertMdToPlainInternal(input_content, input_size, &output_content, &output_size);
            } else {
                convert_result = VTL_publication_res_kError;
            }
            break;

        default:
            convert_result = VTL_publication_res_kError;
            break;
    }

    if (convert_result == VTL_publication_res_kOk) {
        VTL_publication_FileWriteS(config->output_file, output_content, output_size);
    }

    free(input_content);
    if (output_content) {
        free(output_content);
    }

    return convert_result;
}

VTL_publication_app_result VTL_publication_StdMdToTgMdS(const char* std_md, size_t std_md_size, char** tg_md, size_t* tg_md_size) {
    return VTL_publication_ConvertStdMdToTgMdInternal(std_md, std_md_size, tg_md, tg_md_size);
}

VTL_publication_app_result VTL_publication_TgMdToStdMdS(const char* tg_md, size_t tg_md_size, char** std_md, size_t* std_md_size) {
    return VTL_publication_ConvertTgMdToStdMdInternal(tg_md, tg_md_size, std_md, std_md_size);
}

VTL_publication_app_result VTL_publication_MdToPlainS(const char* md, size_t md_size, char** plain, size_t* plain_size) {
    return VTL_publication_ConvertMdToPlainInternal(md, md_size, plain, plain_size);
} 
