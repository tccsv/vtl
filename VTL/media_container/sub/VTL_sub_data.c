#include <VTL/media_container/sub/VTL_sub_data.h>
#include <stdlib.h> // Для malloc, free, realloc, strtod
#include <string.h> // Для memcpy, memset, strncpy, strdup, strtok_r, strstr, strchr
#include <stdio.h>  // Для sscanf, snprintf
#include <ctype.h>  // Для isspace
#include <VTL/media_container/sub/opencl/VTL_sub_opencl_strip_tags.h>

// Прототипы вспомогательных функций времени
static VTL_AppResult VTL_sub_ParseSrtVttTime(const char* time_str, double* out_time);
static VTL_AppResult VTL_sub_ParseAssTime(const char* time_str, double* out_time);

// Функции для VTL_sub_Params
VTL_AppResult VTL_sub_ParamsSetHorizontalAlign(VTL_sub_Params* p_params, VTL_sub_HorizontalAlign align)
{
    VTL_AppResult res = VTL_res_kOk;
    int error = 0;
    if (!p_params) {
        res = VTL_res_kNullArgument;
        error = 1;
    }
    if (!error) {
        p_params->horizontal_align = align;
    }
    return res;
}

VTL_AppResult VTL_sub_ParamsSetColor(VTL_sub_Params* p_params, VTL_sub_ColorRGB color)
{
    VTL_AppResult res = VTL_res_kOk;
    int error = 0;
    if (!p_params) {
        res = VTL_res_kNullArgument;
        error = 1;
    }
    if (!error) {
        p_params->color = color;
    }
    return res;
}

VTL_AppResult VTL_sub_ParamsSetBackgroundColor(VTL_sub_Params* p_params, VTL_sub_ColorRGB background_color)
{
    VTL_AppResult res = VTL_res_kOk;
    int error = 0;
    if (!p_params) {
        res = VTL_res_kNullArgument;
        error = 1;
    }
    if (!error) {
        p_params->background_color = background_color;
    }
    return res;
}

VTL_AppResult VTL_sub_ParamsSetOutlineColor(VTL_sub_Params* p_params, VTL_sub_ColorRGB outline_color)
{
    VTL_AppResult res = VTL_res_kOk;
    int error = 0;
    if (!p_params) {
        res = VTL_res_kNullArgument;
        error = 1;
    }
    if (!error) {
        p_params->outline_color = outline_color;
    }
    return res;
}

VTL_AppResult VTL_sub_ParamsSetTextSize(VTL_sub_Params* p_params, VTL_sub_Size text_size)
{
    VTL_AppResult res = VTL_res_kOk;
    int error = 0;
    if (!p_params) {
        res = VTL_res_kNullArgument;
        error = 1;
    }
    if (!error) {
        p_params->text_size = text_size;
    }
    return res;
}

VTL_AppResult VTL_sub_ParamsSetMargin(VTL_sub_Params* p_params, VTL_sub_Size margin)
{
    VTL_AppResult res = VTL_res_kOk;
    int error = 0;
    if (!p_params) {
        res = VTL_res_kNullArgument;
        error = 1;
    }
    if (!error) {
        p_params->margin = margin;
    }
    return res;
}

VTL_AppResult VTL_sub_ParamsSetFontName(VTL_sub_Params* p_params, VTL_sub_FontName font_name)
{
    VTL_AppResult res = VTL_res_kOk;
    int error = 0;
    if (!p_params || !font_name) {
        res = VTL_res_kNullArgument;
        error = 1;
    }
    if (!error) {
        strncpy(p_params->font_name, font_name, VTL_STANDART_STRING_MAX_LENGTH - 1);
        p_params->font_name[VTL_STANDART_STRING_MAX_LENGTH - 1] = '\0';
    }
    return res;
}

// Функции для VTL_sub_List
VTL_AppResult VTL_sub_ListCreate(VTL_sub_List** pp_sub_list)
{
    if (!pp_sub_list) return VTL_res_kNullArgument;
    *pp_sub_list = (VTL_sub_List*)malloc(sizeof(VTL_sub_List));
    if (!*pp_sub_list) return VTL_res_kAllocError;
    (*pp_sub_list)->entries = NULL;
    (*pp_sub_list)->count = 0;
    return VTL_res_kOk;
}

VTL_AppResult VTL_sub_ListDestroy(VTL_sub_List** pp_sub_list)
{
    if (!pp_sub_list || !*pp_sub_list) return VTL_res_kNullArgument;
    
    for (size_t i = 0; i < (*pp_sub_list)->count; ++i) {
        if ((*pp_sub_list)->entries[i].text) {
            free((*pp_sub_list)->entries[i].text);
        }
        if ((*pp_sub_list)->entries[i].style) {
            free((*pp_sub_list)->entries[i].style);
        }
    }
    free((*pp_sub_list)->entries);
    free(*pp_sub_list);
    *pp_sub_list = NULL;
    return VTL_res_kOk;
}

VTL_AppResult VTL_sub_ListAddEntry(VTL_sub_List* p_sub_list, const VTL_sub_Entry* p_entry)
{
    if (!p_sub_list || !p_entry) return VTL_res_kNullArgument;

    VTL_sub_Entry* new_entries = (VTL_sub_Entry*)realloc(p_sub_list->entries, (p_sub_list->count + 1) * sizeof(VTL_sub_Entry));
    if (!new_entries) return VTL_res_kAllocError;
    
    p_sub_list->entries = new_entries;
    VTL_sub_Entry* new_item = &p_sub_list->entries[p_sub_list->count];

    // Глубокое копирование записи
    new_item->index = p_entry->index;
    new_item->start = p_entry->start;
    new_item->end = p_entry->end;
    
    new_item->text = p_entry->text ? strdup(p_entry->text) : NULL;
    if (p_entry->text && !new_item->text) {
        return VTL_res_kAllocError;
    }

    new_item->style = p_entry->style ? strdup(p_entry->style) : NULL;
    if (p_entry->style && !new_item->style) {
        if(new_item->text) free(new_item->text);
        return VTL_res_kAllocError;
    }
    
    p_sub_list->count++;
    return VTL_res_kOk;
}

VTL_AppResult VTL_sub_ListGetEntry(const VTL_sub_List* p_sub_list, size_t index, VTL_sub_Entry** pp_entry)
{
    if (!p_sub_list || !pp_entry) return VTL_res_kNullArgument;
    if (index >= p_sub_list->count) return VTL_res_kArgumentError;
    
    *pp_entry = (VTL_sub_Entry*)&p_sub_list->entries[index]; // Убираем const для выходного параметра
    return VTL_res_kOk;
}

VTL_AppResult VTL_sub_ListRemoveEntry(VTL_sub_List* p_sub_list, size_t index)
{
    if (!p_sub_list) return VTL_res_kNullArgument;
    if (index >= p_sub_list->count) return VTL_res_kArgumentError;

    if (p_sub_list->entries[index].text) {
        free(p_sub_list->entries[index].text);
    }
    if (p_sub_list->entries[index].style) {
        free(p_sub_list->entries[index].style);
    }

    // Сдвигаем элементы
    if (index < p_sub_list->count - 1) {
        memmove(&p_sub_list->entries[index], &p_sub_list->entries[index + 1], (p_sub_list->count - index - 1) * sizeof(VTL_sub_Entry));
    }
    
    p_sub_list->count--;
    
    if (p_sub_list->count == 0) {
        free(p_sub_list->entries);
        p_sub_list->entries = NULL;
    } else {
        // Пытаемся уменьшить массив, но не считаем критической ошибкой, если realloc вернет NULL
        // так как данные все еще действительны (хотя и с избыточной емкостью).
        VTL_sub_Entry* resized_entries = (VTL_sub_Entry*)realloc(p_sub_list->entries, p_sub_list->count * sizeof(VTL_sub_Entry));
        if (resized_entries) { 
            p_sub_list->entries = resized_entries;
        }
    }
    
    return VTL_res_kOk;
}

// Вспомогательная функция для удаления начальных и конечных пробельных символов
static VTL_AppResult VTL_sub_TrimWhitespace(char* str, char** out_trimmed) {
    if (!str || !out_trimmed) return VTL_res_kNullArgument;
    char *start = str;
    char *end;
    while (isspace((unsigned char)*start)) start++;
    if (*start == 0) { *out_trimmed = start; return VTL_res_kOk; }
    end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
    *out_trimmed = start;
    return VTL_res_kOk;
}

// Вспомогательная функция для удаления ASS-тегов из строки (типа {\b1}, {\c&HFFFFFF&})
static VTL_AppResult VTL_sub_StripAssTags(const char* text_with_tags, char** out_stripped_text) {
    if (!text_with_tags || !out_stripped_text) return VTL_res_kNullArgument;
    size_t len = strlen(text_with_tags);
    char* stripped_text = (char*)malloc(len + 1);
    if (!stripped_text) return VTL_res_kAllocError;
    const char* src = text_with_tags;
    char* dst = stripped_text;
    int in_tag = 0;
    while (*src) {
        if (*src == '{') { in_tag = 1; src++; }
        else if (*src == '}' && in_tag) { in_tag = 0; src++; }
        else { if (!in_tag) *dst++ = *src; src++; }
    }
    *dst = '\0';
    *out_stripped_text = stripped_text;
    return VTL_res_kOk;
}

// Преобразование ARGB (0xAARRGGBB) в ASS (&HAABBGGRR)
VTL_AppResult VTL_sub_ArgbToAssStr(uint32_t argb, char* buf, size_t bufsz) {
    uint8_t a = (argb >> 24) & 0xFF;
    uint8_t r = (argb >> 16) & 0xFF;
    uint8_t g = (argb >> 8) & 0xFF;
    uint8_t b = argb & 0xFF;
    uint8_t a_ass = 0xFF - a;
    int n = snprintf(buf, bufsz, "&H%02X%02X%02X%02X", a_ass, b, g, r);
    if (n < 0 || (size_t)n >= bufsz) return VTL_res_kSubtitleTextOverflow;
    return VTL_res_kOk;
}

// --- Новый код: прототипы вспомогательных функций ---
static VTL_AppResult VTL_sub_ParseSrt(VTL_BufferData* p_buffer_data, VTL_sub_Format format, VTL_sub_List** pp_sub_list);
static VTL_AppResult VTL_sub_ParseAss(VTL_BufferData* p_buffer_data, VTL_sub_List** pp_sub_list);

VTL_AppResult VTL_sub_Parse(VTL_BufferData* p_buffer_data, VTL_sub_Format format, VTL_sub_List** pp_sub_list)
{
    if (!p_buffer_data || !p_buffer_data->data || !pp_sub_list) {
        return VTL_res_kNullArgument;
    }
    *pp_sub_list = NULL;
    if (format == VTL_sub_format_kSRT || format == VTL_sub_format_kVTT) {
        return VTL_sub_ParseSrt(p_buffer_data, format, pp_sub_list);
    } else if (format == VTL_sub_format_kASS) {
        return VTL_sub_ParseAss(p_buffer_data, pp_sub_list);
    } else {
        return VTL_res_kUnsupportedFormat;
    }
}

// --- Новый код: реализация вспомогательных функций ---
static VTL_AppResult VTL_sub_ParseSrt(VTL_BufferData* p_buffer_data, VTL_sub_Format format, VTL_sub_List** pp_sub_list) {
    VTL_AppResult res = VTL_res_kOk;
    int error = 0;
    res = VTL_sub_ListCreate(pp_sub_list);
    if (res != VTL_res_kOk) error = 1;
    char* buffer_copy = NULL;
    if (!error) {
        buffer_copy = strdup(p_buffer_data->data);
        if (!buffer_copy) {
            VTL_sub_ListDestroy(pp_sub_list);
            return VTL_res_kAllocError;
        }
    }
    char* saveptr1 = NULL;
    char* line = NULL;
    VTL_sub_Entry current_entry;
    int state = 0;
    char text_buffer[4096] = {0};
    int auto_increment_index = 1;
    char vtt_id_buffer[256];
    memset(&current_entry, 0, sizeof(VTL_sub_Entry));
    line = strtok_r(buffer_copy, "\n", &saveptr1);
    while (!error && line != NULL) {
        char* trimmed_line = NULL;
        if (VTL_sub_TrimWhitespace(line, &trimmed_line) != VTL_res_kOk) {
            if (buffer_copy) free(buffer_copy);
            if (*pp_sub_list) VTL_sub_ListDestroy(pp_sub_list);
            return VTL_res_kArgumentError;
        }
        line = trimmed_line;
        int skip_line = 0;
        if (strlen(line) == 0) {
            if (state == 2 && strlen(text_buffer) > 0) {
                current_entry.text = strdup(text_buffer);
                if (!current_entry.text) {
                    if (buffer_copy) free(buffer_copy);
                    if (*pp_sub_list) VTL_sub_ListDestroy(pp_sub_list);
                    return VTL_res_kAllocError;
                }
                if(current_entry.index == 0 && format == VTL_sub_format_kSRT) current_entry.index = auto_increment_index++;
                res = VTL_sub_ListAddEntry(*pp_sub_list, &current_entry);
                free(current_entry.text);
                current_entry.text = NULL;
                if (current_entry.style) { free(current_entry.style); current_entry.style = NULL; }
                if (res != VTL_res_kOk) {
                    if (buffer_copy) free(buffer_copy);
                    if (*pp_sub_list) VTL_sub_ListDestroy(pp_sub_list);
                    return res;
                }
                memset(&current_entry, 0, sizeof(VTL_sub_Entry));
                memset(text_buffer, 0, sizeof(text_buffer));
                state = 0;
            }
            skip_line = 1;
        }
        int is_new_index = 0, is_new_time = 0;
        int parsed_index = -1;
        char time_start_str[30], time_end_str[30];
        if (!skip_line && sscanf(line, "%d", &parsed_index) == 1 && strstr(line, "-->") == NULL) is_new_index = 1;
        if (!skip_line && sscanf(line, "%29s --> %29s", time_start_str, time_end_str) == 2 && strstr(line, "-->") != NULL) is_new_time = 1;
        if (!skip_line && (is_new_index || is_new_time) && state == 2 && strlen(text_buffer) > 0) {
            current_entry.text = strdup(text_buffer);
            if (!current_entry.text) {
                if (buffer_copy) free(buffer_copy);
                if (*pp_sub_list) VTL_sub_ListDestroy(pp_sub_list);
                return VTL_res_kAllocError;
            }
            if(current_entry.index == 0 && format == VTL_sub_format_kSRT) current_entry.index = auto_increment_index++;
            res = VTL_sub_ListAddEntry(*pp_sub_list, &current_entry);
            free(current_entry.text);
            current_entry.text = NULL;
            if (current_entry.style) { free(current_entry.style); current_entry.style = NULL; }
            if (res != VTL_res_kOk) {
                if (buffer_copy) free(buffer_copy);
                if (*pp_sub_list) VTL_sub_ListDestroy(pp_sub_list);
                return res;
            }
            memset(&current_entry, 0, sizeof(VTL_sub_Entry));
            memset(text_buffer, 0, sizeof(text_buffer));
            state = 0;
        }
        if (!skip_line && state == 0) {
            if (is_new_index) {
                current_entry.index = parsed_index;
                state = 1;
            } else if (is_new_time) {
                double start = 0, end = 0;
                if (VTL_sub_ParseSrtVttTime(time_start_str, &start) != VTL_res_kOk ||
                    VTL_sub_ParseSrtVttTime(time_end_str, &end) != VTL_res_kOk) {
                    state = 0; memset(&current_entry, 0, sizeof(VTL_sub_Entry));
                } else {
                    current_entry.start = start;
                    current_entry.end = end;
                    if (current_entry.start >= 0 && current_entry.end >= 0 && current_entry.end >= current_entry.start) {
                        state = 2;
                        if (format == VTL_sub_format_kVTT) current_entry.index = 0;
                        else if (current_entry.index == 0) current_entry.index = auto_increment_index++;
                        memset(text_buffer, 0, sizeof(text_buffer));
                    } else {
                        state = 0; memset(&current_entry, 0, sizeof(VTL_sub_Entry));
                    }
                }
            } else if (format == VTL_sub_format_kVTT) {
                if (strlen(line) < sizeof(vtt_id_buffer)) {
                    current_entry.style = strdup(line);
                    if(!current_entry.style) {
                        if (buffer_copy) free(buffer_copy);
                        if (*pp_sub_list) VTL_sub_ListDestroy(pp_sub_list);
                        return VTL_res_kAllocError;
                    }
                    current_entry.index = 0;
                    state = 1;
                } else {
                    state = 0; memset(&current_entry, 0, sizeof(VTL_sub_Entry));
                }
            } else {
                state = 0; memset(&current_entry, 0, sizeof(VTL_sub_Entry));
            }
        } else if (!skip_line && state == 1) {
            char time_start_str2[30], time_end_str2[30];
            if (sscanf(line, "%29s --> %29s", time_start_str2, time_end_str2) == 2 && strstr(line, "-->") != NULL) {
                double start = 0, end = 0;
                if (VTL_sub_ParseSrtVttTime(time_start_str2, &start) != VTL_res_kOk ||
                    VTL_sub_ParseSrtVttTime(time_end_str2, &end) != VTL_res_kOk) {
                    state = 0;
                    if (current_entry.style) { free(current_entry.style); current_entry.style = NULL; }
                    memset(&current_entry, 0, sizeof(VTL_sub_Entry));
                } else {
                    current_entry.start = start;
                    current_entry.end = end;
                    if (current_entry.start < 0 || current_entry.end < 0 || current_entry.end < current_entry.start) {
                        state = 0;
                        if (current_entry.style) { free(current_entry.style); current_entry.style = NULL; }
                        memset(&current_entry, 0, sizeof(VTL_sub_Entry));
                    } else {
                        state = 2;
                        if (format == VTL_sub_format_kSRT && current_entry.index == 0) current_entry.index = auto_increment_index++;
                        memset(text_buffer, 0, sizeof(text_buffer));
                    }
                }
            } else {
                state = 0;
                if (current_entry.style) { free(current_entry.style); current_entry.style = NULL; }
                memset(&current_entry, 0, sizeof(VTL_sub_Entry));
            }
        } else if (!skip_line && state == 2) {
            if (strlen(text_buffer) + strlen(line) + 2 < sizeof(text_buffer)) {
                if (strlen(text_buffer) > 0) {
                    strcat(text_buffer, "\n");
                }
                strcat(text_buffer, line);
            } else {
                if (buffer_copy) free(buffer_copy);
                if (*pp_sub_list) VTL_sub_ListDestroy(pp_sub_list);
                return VTL_res_kSubtitleTextOverflow;
            }
        }
        if (!error) {
            line = strtok_r(NULL, "\n", &saveptr1);
        }
    }
    if (!error && state == 2 && strlen(text_buffer) > 0) {
        current_entry.text = strdup(text_buffer);
        if (!current_entry.text) {
            if (buffer_copy) free(buffer_copy);
            if (*pp_sub_list) VTL_sub_ListDestroy(pp_sub_list);
            return VTL_res_kAllocError;
        }
        if(current_entry.index == 0 && format == VTL_sub_format_kSRT) current_entry.index = auto_increment_index++;
        res = VTL_sub_ListAddEntry(*pp_sub_list, &current_entry);
        free(current_entry.text);
        current_entry.text = NULL;
        if (current_entry.style) { free(current_entry.style); current_entry.style = NULL; }
        if (res != VTL_res_kOk) {
            if (buffer_copy) free(buffer_copy);
            if (*pp_sub_list) VTL_sub_ListDestroy(pp_sub_list);
            return res;
        }
    }
    if (buffer_copy) free(buffer_copy);
    if (error) {
        if (*pp_sub_list) VTL_sub_ListDestroy(pp_sub_list);
    }
    return res;
}

static VTL_AppResult VTL_sub_ParseAss(VTL_BufferData* p_buffer_data, VTL_sub_List** pp_sub_list) {
    VTL_AppResult res = VTL_res_kOk;
    int error = 0;
    res = VTL_sub_ListCreate(pp_sub_list);
    if (res != VTL_res_kOk) error = 1;
    char* buffer_copy = strdup(p_buffer_data->data);
    if (!buffer_copy) {
        VTL_sub_ListDestroy(pp_sub_list);
        return VTL_res_kAllocError;
    }
    char* saveptr_line;
    char* line = strtok_r(buffer_copy, "\n", &saveptr_line);
    VTL_sub_Entry current_entry;
    int auto_ass_index = 1;
    enum AssSection { SECTION_NONE, SECTION_SCRIPT_INFO, SECTION_STYLES, SECTION_EVENTS } current_section = SECTION_NONE;
    int format_start = -1, format_end = -1, format_style = -1, format_text = -1;
    int num_event_format_fields = 0;
    while (!error && line != NULL) {
        char* trimmed_line = NULL;
        if (VTL_sub_TrimWhitespace(line, &trimmed_line) != VTL_res_kOk) {
            res = VTL_res_kArgumentError; error = 1;
        } else {
            line = trimmed_line;
            int skip_line = 0;
            if (line[0] == '[' && line[strlen(line)-1] == ']') { 
                char* trimmed_section_line = line;
                VTL_sub_TrimWhitespace(line, &trimmed_section_line);
                if (strcmp(trimmed_section_line, "[Script Info]") == 0) current_section = SECTION_SCRIPT_INFO;
                else if (strcmp(trimmed_section_line, "[V4+ Styles]") == 0) current_section = SECTION_STYLES;
                else if (strcmp(trimmed_section_line, "[Events]") == 0) current_section = SECTION_EVENTS;
                else current_section = SECTION_NONE;
                skip_line = 1;
            }
            // --- обработка формата событий ---
            if (!skip_line && current_section == SECTION_EVENTS && strncmp(line, "Format:", 7) == 0) {
                char* format_line = line + 7;
                char* token;
                int idx = 0;
                char* saveptr_fmt;
                token = strtok_r(format_line, ",", &saveptr_fmt);
                while (token) {
                    char* trimmed_token = token;
                    VTL_sub_TrimWhitespace(token, &trimmed_token);
                    if (strcasecmp(trimmed_token, "Start") == 0) format_start = idx;
                    else if (strcasecmp(trimmed_token, "End") == 0) format_end = idx;
                    else if (strcasecmp(trimmed_token, "Style") == 0) format_style = idx;
                    else if (strcasecmp(trimmed_token, "Text") == 0) format_text = idx;
                    idx++;
                    token = strtok_r(NULL, ",", &saveptr_fmt);
                }
                num_event_format_fields = idx;
                skip_line = 1;
            }
            // --- обработка диалогов ---
            if (!skip_line && current_section == SECTION_EVENTS && strncmp(line, "Dialogue:", 9) == 0) {
                if (format_start == -1 || format_end == -1 || format_text == -1 || num_event_format_fields == 0) {
                    skip_line = 1;
                } else {
                    memset(&current_entry, 0, sizeof(VTL_sub_Entry));
                    char* dialogue_line_ptr = line + 9; 
                    char* fields[64]; 
                    int field_count = 0;
                    char* current_pos = dialogue_line_ptr;
                    int N = num_event_format_fields; 
                    for (int k = 0; k < N-1; ++k) {
                        if (current_pos == NULL || *current_pos == '\0') {
                            fields[field_count++] = "";
                        } else {
                            char* comma_pos = strchr(current_pos, ',');
                            if (comma_pos) {
                                *comma_pos = '\0';
                                char* trimmed_field = current_pos;
                                VTL_sub_TrimWhitespace(current_pos, &trimmed_field);
                                fields[field_count++] = trimmed_field;
                                current_pos = comma_pos + 1;
                            } else {
                                char* trimmed_field = current_pos;
                                VTL_sub_TrimWhitespace(current_pos, &trimmed_field);
                                fields[field_count++] = trimmed_field;
                                current_pos = NULL;
                            }
                        }
                    }
                    while (field_count < N-1) {
                        fields[field_count++] = "";
                    }
                    if (current_pos) {
                        size_t len = strlen(current_pos);
                        while (len > 0 && (current_pos[len-1] == '\n' || current_pos[len-1] == '\r' || current_pos[len-1] == ' ')) {
                            current_pos[--len] = '\0';
                        }
                        fields[field_count++] = current_pos;
                    } else {
                        fields[field_count++] = "";
                    }
                    if (field_count == num_event_format_fields) {
                        double start = 0, end = 0;
                        char* start_field = fields[format_start];
                        char* end_field = fields[format_end];
                        char* trimmed_start = start_field, *trimmed_end = end_field;
                        VTL_sub_TrimWhitespace(start_field, &trimmed_start);
                        VTL_sub_TrimWhitespace(end_field, &trimmed_end);
                        if (!((format_start < field_count && VTL_sub_ParseAssTime(trimmed_start, &start) == VTL_res_kOk) &&
                            (format_end < field_count && VTL_sub_ParseAssTime(trimmed_end, &end) == VTL_res_kOk))) {
                            // skip
                        } else {
                            current_entry.start = start;
                            current_entry.end = end;
                            char* raw_text = (format_text < field_count) ? fields[format_text] : NULL;
                            if (raw_text) {
                                char* cleaned_text = NULL;
                                if (VTL_sub_StripAssTags(raw_text, &cleaned_text) == VTL_res_kOk && cleaned_text) {
                                    current_entry.text = strdup(cleaned_text);
                                    free(cleaned_text);
                                    if (!current_entry.text) { res = VTL_res_kAllocError; error = 1; }
                                } else {
                                    res = VTL_res_kAllocError; error = 1;
                                }
                            } else {
                                res = VTL_res_kAllocError; error = 1;
                            }
                            if (!error && current_entry.text) {
                                res = VTL_sub_ListAddEntry(*pp_sub_list, &current_entry);
                                if (res != VTL_res_kOk) { error = 1; }
                                if (current_entry.text) { free(current_entry.text); current_entry.text = NULL; }
                                if (current_entry.style) { free(current_entry.style); current_entry.style = NULL; }
                            }
                        }
                    }
                }
            }
        }
        if (!error) {
            line = strtok_r(NULL, "\n", &saveptr_line);
        }
    }
    if (buffer_copy) free(buffer_copy);
    if (error) {
        if (*pp_sub_list) VTL_sub_ListDestroy(pp_sub_list);
    }
    return res;
}

// Вспомогательная функция для форматирования секунд в строку HH:MM:SS,ms (SRT), HH:MM:SS.ms (VTT), H:MM:SS.ss (ASS)
static VTL_AppResult VTL_sub_FormatSubTime(double time_seconds, char* buffer, size_t buffer_size, VTL_sub_Format format) {
    if (!buffer || buffer_size < 12) return VTL_res_kArgumentError;
    if (time_seconds < 0) time_seconds = 0;
    int hours = (int)(time_seconds / 3600);
    double remaining_seconds = time_seconds - hours * 3600.0;
    int minutes = (int)(remaining_seconds / 60.0);
    remaining_seconds -= minutes * 60.0;
    int seconds = (int)remaining_seconds;
    int n = 0;
    if (format == VTL_sub_format_kASS) {
        int centiseconds = (int)((remaining_seconds - seconds) * 100.0);
        n = snprintf(buffer, buffer_size, "%d:%02d:%02d.%02d", hours, minutes, seconds, centiseconds);
    } else {
        int milliseconds = (int)((remaining_seconds - seconds) * 1000.0);
        char separator = (format == VTL_sub_format_kVTT) ? '.' : ',';
        n = snprintf(buffer, buffer_size, "%02d:%02d:%02d%c%03d", hours, minutes, seconds, separator, milliseconds);
    }
    if (n < 0 || (size_t)n >= buffer_size) return VTL_res_kSubtitleTextOverflow;
    return VTL_res_kOk;
}

typedef struct {
    char** output_string;
    size_t* current_length;
    size_t* current_capacity;
} VTL_sub_OutputBufferCtx;

static VTL_AppResult VTL_sub_AppendToOutput(VTL_sub_OutputBufferCtx* ctx, const char* str_to_add) {
    size_t len_to_add = strlen(str_to_add);
    if (*(ctx->current_length) + len_to_add + 1 > *(ctx->current_capacity)) {
        *(ctx->current_capacity) = (*(ctx->current_length) + len_to_add + 1) * 2;
        char* new_output_string = (char*)realloc(*(ctx->output_string), *(ctx->current_capacity));
        if (!new_output_string) {
            return VTL_res_kAllocError;
        }
        *(ctx->output_string) = new_output_string;
    }
    strcat(*(ctx->output_string), str_to_add);
    *(ctx->current_length) += len_to_add;
    return VTL_res_kOk;
}

VTL_AppResult VTL_sub_FormatToString(const VTL_sub_List* p_sub_list, VTL_sub_Format format, VTL_BufferData** pp_buffer_data, const VTL_sub_StyleParams* style_params)
{
    if (!p_sub_list || !pp_buffer_data) return VTL_res_kNullArgument;

    *pp_buffer_data = (VTL_BufferData*)malloc(sizeof(VTL_BufferData));
    if (!*pp_buffer_data) return VTL_res_kAllocError;
    (*pp_buffer_data)->data = NULL;
    (*pp_buffer_data)->data_size = 0;

    size_t current_capacity = 2048;
    char* output_string = (char*)malloc(current_capacity);
    if (!output_string) {
        free(*pp_buffer_data);
        *pp_buffer_data = NULL;
        return VTL_res_kAllocError;
    }
    output_string[0] = '\0';
    size_t current_length = 0;
    VTL_sub_OutputBufferCtx ctx = { &output_string, &current_length, &current_capacity };

    if (format == VTL_sub_format_kVTT) {
        if (VTL_sub_AppendToOutput(&ctx, "WEBVTT\n\n") != VTL_res_kOk) return VTL_res_kAllocError;
    } else if (format == VTL_sub_format_kASS) {
        if (VTL_sub_AppendToOutput(&ctx, "[Script Info]\n") != VTL_res_kOk) return VTL_res_kAllocError;
        if (VTL_sub_AppendToOutput(&ctx, "Title: Subtitles\n") != VTL_res_kOk) return VTL_res_kAllocError;
        if (VTL_sub_AppendToOutput(&ctx, "ScriptType: v4.00+\n") != VTL_res_kOk) return VTL_res_kAllocError;
        if (VTL_sub_AppendToOutput(&ctx, "WrapStyle: 0\n") != VTL_res_kOk) return VTL_res_kAllocError;
        if (VTL_sub_AppendToOutput(&ctx, "PlayResX: 384\n") != VTL_res_kOk) return VTL_res_kAllocError;
        if (VTL_sub_AppendToOutput(&ctx, "PlayResY: 288\n") != VTL_res_kOk) return VTL_res_kAllocError;
        if (VTL_sub_AppendToOutput(&ctx, "\n") != VTL_res_kOk) return VTL_res_kAllocError;

        if (VTL_sub_AppendToOutput(&ctx, "[V4+ Styles]\n") != VTL_res_kOk) return VTL_res_kAllocError;
        if (VTL_sub_AppendToOutput(&ctx, "Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding\n") != VTL_res_kOk) return VTL_res_kAllocError;
        char style_line[512];
        if (style_params) {
            fprintf(stderr, "[DEBUG] FormatToString: primary_color=0x%08X\n", style_params->primary_color);
            char color_buf[16], back_buf[16], outline_buf[16];
            if (VTL_sub_ArgbToAssStr(style_params->primary_color, color_buf, sizeof(color_buf)) != VTL_res_kOk ||
                VTL_sub_ArgbToAssStr(style_params->back_color, back_buf, sizeof(back_buf)) != VTL_res_kOk ||
                VTL_sub_ArgbToAssStr(style_params->outline_color, outline_buf, sizeof(outline_buf)) != VTL_res_kOk) {
                if (output_string) free(output_string);
                if (*pp_buffer_data) {
                    free(*pp_buffer_data);
                    *pp_buffer_data = NULL;
                }
                return VTL_res_kSubtitleTextOverflow;
            }
            snprintf(style_line, sizeof(style_line),
                "Style: Default,%s,%d,%s,%s,%s,%s,%d,%d,%d,%d,100,100,0,0,%d,%d,%d,%d,%d,%d,%d,1\n",
                style_params->fontname ? style_params->fontname : "Arial",
                style_params->fontsize > 0 ? style_params->fontsize : 20,
                color_buf,
                color_buf,
                outline_buf,
                back_buf,
                style_params->bold ? -1 : 0,
                style_params->italic,
                style_params->underline,
                0,
                style_params->border_style,
                style_params->outline,
                2,
                style_params->alignment > 0 ? style_params->alignment : 2,
                style_params->margin_l,
                style_params->margin_r,
                style_params->margin_v
            );
        } else {
            snprintf(style_line, sizeof(style_line), "Style: Default,Arial,20,&H00FFFFFF,&H000000FF,&H00000000,&H00000000,0,0,0,0,100,100,0,0,1,2,2,2,10,10,10,1\n");
        }
        if (VTL_sub_AppendToOutput(&ctx, style_line) != VTL_res_kOk) return VTL_res_kAllocError;
        if (VTL_sub_AppendToOutput(&ctx, "\n") != VTL_res_kOk) return VTL_res_kAllocError;
        
        if (VTL_sub_AppendToOutput(&ctx, "[Events]\n") != VTL_res_kOk) return VTL_res_kAllocError;
        if (VTL_sub_AppendToOutput(&ctx, "Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n") != VTL_res_kOk) return VTL_res_kAllocError;
        for (size_t i = 0; i < p_sub_list->count; ++i) {
            char start_buf[32], end_buf[32];
            if (VTL_sub_FormatSubTime(p_sub_list->entries[i].start, start_buf, sizeof(start_buf), VTL_sub_format_kASS) != VTL_res_kOk ||
                VTL_sub_FormatSubTime(p_sub_list->entries[i].end, end_buf, sizeof(end_buf), VTL_sub_format_kASS) != VTL_res_kOk) {
                continue;
            }
            char dialogue_line[4096];
            snprintf(dialogue_line, sizeof(dialogue_line),
                "Dialogue: 0,%s,%s,Default,,0,0,0,,%s\n",
                start_buf, end_buf, p_sub_list->entries[i].text ? p_sub_list->entries[i].text : "");
            VTL_sub_AppendToOutput(&ctx, dialogue_line);
        }
    }

    if (format == VTL_sub_format_kSRT || format == VTL_sub_format_kVTT || format == VTL_sub_format_kASS) {
        if (format == VTL_sub_format_kASS && p_sub_list->count > 0) {
        }
    }

    (*pp_buffer_data)->data = output_string;
    (*pp_buffer_data)->data_size = current_length;
    return VTL_res_kOk;
}

// Вспомогательная функция для определения формата субтитров по расширению файла
static VTL_sub_Format VTL_sub_DetectFormatFromExtension(const char* file_path) {
    if (!file_path) return VTL_sub_format_kUNKNOWN;
    const char *dot = strrchr(file_path, '.');
    if (!dot || dot == file_path) return VTL_sub_format_kUNKNOWN;

    if (strcmp(dot, ".srt") == 0) return VTL_sub_format_kSRT;
    if (strcmp(dot, ".ass") == 0) return VTL_sub_format_kASS;
    if (strcmp(dot, ".ssa") == 0) return VTL_sub_format_kASS;
    if (strcmp(dot, ".vtt") == 0) return VTL_sub_format_kVTT;

    return VTL_sub_format_kUNKNOWN;
}

// Функции для загрузки и сохранения списка субтитров из/в файл
VTL_AppResult VTL_sub_LoadFromFile(const char* file_path, VTL_sub_Format* detected_format_out, VTL_sub_List** pp_sub_list)
{
    if (!file_path || !pp_sub_list || !detected_format_out) return VTL_res_kNullArgument;

    *pp_sub_list = NULL;
    VTL_sub_Format format = VTL_sub_DetectFormatFromExtension(file_path);
    *detected_format_out = format;

    if (format == VTL_sub_format_kUNKNOWN) {
        return VTL_res_kUnsupportedFormat;
    }

    FILE* fp = fopen(file_path, "rb");
    if (!fp) {
        return VTL_res_kIOError;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size <= 0 && file_size != 0) {
        fclose(fp);
        if (file_size == 0) return VTL_sub_ListCreate(pp_sub_list);
        return VTL_res_kIOError;
    }
    
    VTL_BufferData buffer_data;
    buffer_data.data = (char*)malloc(file_size + 1);
    if (!buffer_data.data) {
        fclose(fp);
        return VTL_res_kAllocError;
    }

    size_t bytes_read = fread(buffer_data.data, 1, file_size, fp);
    fclose(fp);

    if (bytes_read != (size_t)file_size) {
        free(buffer_data.data);
        return VTL_res_kIOError;
    }
    buffer_data.data[file_size] = '\0';
    buffer_data.data_size = file_size;

    VTL_AppResult parse_res = VTL_sub_Parse(&buffer_data, format, pp_sub_list);
    
    free(buffer_data.data);

    return parse_res;
}

VTL_AppResult VTL_sub_SaveToFile(const char* file_path, VTL_sub_Format format, const VTL_sub_List* p_sub_list, const VTL_sub_StyleParams* style_params)
{
    if (!file_path || !p_sub_list) return VTL_res_kNullArgument;

    VTL_sub_Format target_format = format;
    if (target_format == VTL_sub_format_kUNKNOWN) {
        target_format = VTL_sub_DetectFormatFromExtension(file_path);
        if (target_format == VTL_sub_format_kUNKNOWN) {
            return VTL_res_kUnsupportedFormat;
        }
    }
    VTL_BufferData* buffer_data = NULL;
    VTL_AppResult format_res = VTL_sub_FormatToString(p_sub_list, target_format, &buffer_data, style_params);
    if (format_res != VTL_res_kOk) {
        if (buffer_data) {
            free(buffer_data->data);
            free(buffer_data);
        }
        return format_res;
    }

    FILE* fp = fopen(file_path, "wb");
    if (!fp) {
        if (buffer_data) {
            free(buffer_data->data);
            free(buffer_data);
        }
        return VTL_res_kIOError;
    }

    if (buffer_data && buffer_data->data && buffer_data->data_size > 0) {
        size_t bytes_written = fwrite(buffer_data->data, 1, buffer_data->data_size, fp);
        if (bytes_written != buffer_data->data_size) {
            fclose(fp);
            free(buffer_data->data);
            free(buffer_data);
            return VTL_res_kIOError;
        }
    }

    fclose(fp);
    if (buffer_data) {
        free(buffer_data->data);
        free(buffer_data);
    }

    return VTL_res_kOk;
} 

static VTL_AppResult VTL_sub_ParseSrtVttTime(const char* time_str, double* out_time) {
    if (!time_str || !out_time) return VTL_res_kNullArgument;
    int h, m, s, ms;
    if (sscanf(time_str, "%d:%d:%d,%d", &h, &m, &s, &ms) == 4 || 
        sscanf(time_str, "%d:%d:%d.%d", &h, &m, &s, &ms) == 4) {
        *out_time = h * 3600.0 + m * 60.0 + s + ms / 1000.0;
        return VTL_res_kOk;
    }
    return VTL_res_kSubtitleTimeParseError;
}

static VTL_AppResult VTL_sub_ParseAssTime(const char* time_str, double* out_time) {
    if (!time_str || !out_time) return VTL_res_kNullArgument;
    int h, m, s, cs;
    if (sscanf(time_str, "%d:%d:%d.%d", &h, &m, &s, &cs) == 4) {
        *out_time = h * 3600.0 + m * 60.0 + s + cs / 100.0;
        return VTL_res_kOk;
    }
    return VTL_res_kSubtitleTimeParseError;
} 
