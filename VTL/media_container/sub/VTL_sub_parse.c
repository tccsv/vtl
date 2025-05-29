#include <VTL/media_container/sub/VTL_sub_parse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Получить расширение файла
static const char* VTL_sub_GetFileExt(const char* filename) {
    const char* dot = strrchr(filename, '.');
    return dot ? dot + 1 : "";
}

// Определяет формат субтитров по расширению файла
VTL_AppResult VTL_sub_ParseDetectFormat(const char* filename, VTL_sub_Format* out_format) {
    if (!filename || !out_format) return VTL_res_kNullArgument;
    const char* dot = strrchr(filename, '.');
    const char* ext = dot ? dot + 1 : "";
    if (strcasecmp(ext, "srt") == 0) *out_format = VTL_sub_format_kSRT;
    else if (strcasecmp(ext, "ass") == 0) *out_format = VTL_sub_format_kASS;
    else if (strcasecmp(ext, "vtt") == 0) *out_format = VTL_sub_format_kVTT;
    else *out_format = VTL_sub_format_kUNKNOWN;
    return VTL_res_kOk;
}

// Очистка памяти списка субтитров
VTL_AppResult VTL_sub_ListFree(VTL_sub_List* list) {
    if (!list) return VTL_res_kNullArgument;
    if (!list->entries) return VTL_res_kOk;
    for (size_t i = 0; i < list->count; ++i) {
        free(list->entries[i].text);
        free(list->entries[i].style);
    }
    free(list->entries);
    list->entries = NULL;
    list->count = 0;
    return VTL_res_kOk;
}

// Парсер времени
static VTL_AppResult VTL_sub_ParseTime(const char* str, VTL_sub_Format format, double* out_time) {
    int h = 0, m = 0, s = 0, frac = 0;
    if (!str || !out_time) return VTL_res_kNullArgument;
    *out_time = 0.0;
    int parsed = 0;
    if (format == VTL_sub_format_kSRT) {
        parsed = sscanf(str, "%d:%d:%d,%d", &h, &m, &s, &frac);
        if (parsed == 4) {
            *out_time = h * 3600.0 + m * 60.0 + s + frac / 1000.0;
            return VTL_res_kOk;
        }
    } else if (format == VTL_sub_format_kVTT) {
        parsed = sscanf(str, "%d:%d:%d.%d", &h, &m, &s, &frac);
        if (parsed == 4) {
            *out_time = h * 3600.0 + m * 60.0 + s + frac / 1000.0;
            return VTL_res_kOk;
        }
    } else if (format == VTL_sub_format_kASS) {
        parsed = sscanf(str, "%d:%d:%d.%d", &h, &m, &s, &frac);
        if (parsed == 4) {
            *out_time = h * 3600.0 + m * 60.0 + s + frac / 100.0;
            return VTL_res_kOk;
        }
    }
    return VTL_res_kSubtitleTimeParseError;
}

// Вспомогательная функция для изменения размера массива субтитров
static VTL_AppResult VTL_sub_ResizeSubEntryArray(VTL_sub_Entry** arr_ptr, size_t* capacity_ptr, size_t current_len) {
    if (!arr_ptr || !capacity_ptr) return VTL_res_kArgumentError; // Проверяем указатели
    // Если *arr_ptr это NULL, это может быть начальное выделение, current_len должен быть 0
    if (!*arr_ptr && current_len != 0) return VTL_res_kArgumentError; 
    if (*arr_ptr && current_len == 0) { /* Это может быть ок, если просто уменьшаем capacity до 0 */ }

    if (current_len < *capacity_ptr && *arr_ptr != NULL) { // Добавил проверку *arr_ptr != NULL
        return VTL_res_kOk; // Вместимости достаточно
    }
    size_t new_capacity = *capacity_ptr ? (*capacity_ptr) * 2 : 16;
    VTL_sub_Entry* temp_arr = realloc(*arr_ptr, new_capacity * sizeof(VTL_sub_Entry));
    if (!temp_arr) {
        // Вызывающая функция должна будет сама освободить *arr_ptr, если он не NULL, перед возвратом ошибки.
        // Это важно, так как VTL_sub_ListFree ожидает VTL_sub_List.
        return VTL_res_kMemoryError;
    }
    *arr_ptr = temp_arr;
    *capacity_ptr = new_capacity;
    return VTL_res_kOk;
}

// Вспомогательная функция для чтения текстового блока субтитра
static void VTL_sub_ReadSubtitleTextBlock(FILE* f, char* textbuf, size_t buf_size) {
    char line_reader[1024]; // Буфер для чтения строк из файла
    char* p = textbuf;
    *p = '\0'; // Инициализация textbuf пустой строкой

    int done = 0;
    while (!done && fgets(line_reader, sizeof(line_reader), f)) {
        if (line_reader[0] == '\n' || line_reader[0] == '\r') { // Пустая строка означает конец блока
            done = 1;
        } else {
            size_t line_len = strlen(line_reader);
            // Проверка на переполнение textbuf, оставляем место для null-терминатора
            if ((p + line_len) >= (textbuf + buf_size - 1)) {
                size_t remaining_space = (textbuf + buf_size - 1) - p;
                if (remaining_space > 0) {
                    memcpy(p, line_reader, remaining_space);
                    p += remaining_space;
                }
                done = 1; // Буфер заполнен
            } else {
                memcpy(p, line_reader, line_len);
                p += line_len;
            }
        }
    }
    // Удаляем возможные последние переносы строки из собранного textbuf
    while (p > textbuf && (*(p - 1) == '\n' || *(p - 1) == '\r')) {
        p--;
    }
    *p = '\0'; // Завершаем строку текста
}

static VTL_AppResult VTL_sub_ParseFileSRT(FILE* f, VTL_sub_Entry** arr_ptr, size_t* arr_cap_ptr, size_t* arr_len_ptr);
static VTL_AppResult VTL_sub_ParseFileVTT(FILE* f, VTL_sub_Entry** arr_ptr, size_t* arr_cap_ptr, size_t* arr_len_ptr);
static VTL_AppResult VTL_sub_ParseFileASS(FILE* f, VTL_sub_Entry** arr_ptr, size_t* arr_cap_ptr, size_t* arr_len_ptr);

// Парсинг субтитров из файла в список
VTL_AppResult VTL_sub_ParseFile(const char* input_file, VTL_sub_Format input_format, VTL_sub_List* out_list) {
    if (!out_list) return VTL_res_kArgumentError;
    out_list->entries = NULL;
    out_list->count = 0;

    FILE* f = fopen(input_file, "r");
    if (!f) return VTL_res_video_fs_r_kMissingFileErr;

    VTL_sub_Entry* arr = NULL;
    size_t arr_cap = 0;
    size_t arr_len = 0;
    VTL_AppResult res = VTL_res_kOk;

    if (input_format == VTL_sub_format_kSRT) {
        res = VTL_sub_ParseFileSRT(f, &arr, &arr_cap, &arr_len);
    } else if (input_format == VTL_sub_format_kVTT) {
        res = VTL_sub_ParseFileVTT(f, &arr, &arr_cap, &arr_len);
    } else if (input_format == VTL_sub_format_kASS) {
        res = VTL_sub_ParseFileASS(f, &arr, &arr_cap, &arr_len);
    } else {
        fclose(f);
        out_list->entries = NULL;
        out_list->count = 0;
        return VTL_res_kUnsupportedFormat;
    }
    fclose(f);
    if (res != VTL_res_kOk) {
        if (arr != NULL) {
            VTL_sub_List tempList = { .entries = arr, .count = arr_len };
            VTL_sub_ListFree(&tempList);
        }
        out_list->entries = NULL;
        out_list->count = 0;
        return res;
    }
    out_list->entries = arr;
    out_list->count = arr_len;
    return VTL_res_kOk;
}

static VTL_AppResult VTL_sub_ParseFileSRT(FILE* f, VTL_sub_Entry** arr_ptr, size_t* arr_cap_ptr, size_t* arr_len_ptr) {
    char line_buffer[1024];
    char text_buffer[2048];
    VTL_AppResult res = VTL_res_kOk;
    size_t arr_len = 0;
    while (fgets(line_buffer, sizeof(line_buffer), f)) {
        int skip = 0;
        if (line_buffer[0] == '\n' || line_buffer[0] == '\r') skip = 1;
        int idx = atoi(line_buffer);
        if (!skip && idx == 0 && *arr_len_ptr > 0) skip = 1;
        if (!skip) {
            if (!fgets(line_buffer, sizeof(line_buffer), f)) return VTL_res_kParseError;
            char* arrow = strstr(line_buffer, "-->");
            if (!arrow) return VTL_res_kSubtitleFormatError;
            *arrow = '\0';
            double start, end;
            res = VTL_sub_ParseTime(line_buffer, VTL_sub_format_kSRT, &start);
            if (res != VTL_res_kOk) return res;
            res = VTL_sub_ParseTime(arrow + 3, VTL_sub_format_kSRT, &end);
            if (res != VTL_res_kOk) return res;
            VTL_sub_ReadSubtitleTextBlock(f, text_buffer, sizeof(text_buffer));
            res = VTL_sub_ResizeSubEntryArray(arr_ptr, arr_cap_ptr, *arr_len_ptr);
            if (res != VTL_res_kOk) return res;
            (*arr_ptr)[*arr_len_ptr].index = idx;
            (*arr_ptr)[*arr_len_ptr].start = start;
            (*arr_ptr)[*arr_len_ptr].end = end;
            (*arr_ptr)[*arr_len_ptr].text = strdup(text_buffer);
            if (!(*arr_ptr)[*arr_len_ptr].text) return VTL_res_kAllocError;
            (*arr_ptr)[*arr_len_ptr].style = NULL;
            (*arr_len_ptr)++;
        }
    }
    return VTL_res_kOk;
}

static VTL_AppResult VTL_sub_ParseFileVTT(FILE* f, VTL_sub_Entry** arr_ptr, size_t* arr_cap_ptr, size_t* arr_len_ptr) {
    char line_buffer[1024];
    char text_buffer[2048];
    VTL_AppResult res = VTL_res_kOk;
    int found_webvtt = 0;
    while (fgets(line_buffer, sizeof(line_buffer), f) && !found_webvtt) {
        if (strstr(line_buffer, "WEBVTT")) found_webvtt = 1;
    }
    int current_vtt_index = 1;
    while (fgets(line_buffer, sizeof(line_buffer), f)) {
        int skip = 0;
        if (strncmp(line_buffer, "NOTE", 4) == 0 || line_buffer[0] == '\n' || line_buffer[0] == '\r') skip = 1;
        if (!skip) {
            char* arrow = strstr(line_buffer, "-->");
            if (!arrow) {
                if (!fgets(line_buffer, sizeof(line_buffer), f)) return VTL_res_kParseError;
                arrow = strstr(line_buffer, "-->");
                if (!arrow) skip = 1;
            }
            if (!skip && arrow) {
                *arrow = '\0';
                double start, end;
                res = VTL_sub_ParseTime(line_buffer, VTL_sub_format_kVTT, &start);
                if (res != VTL_res_kOk) return res;
                res = VTL_sub_ParseTime(arrow + 3, VTL_sub_format_kVTT, &end);
                if (res != VTL_res_kOk) return res;
                VTL_sub_ReadSubtitleTextBlock(f, text_buffer, sizeof(text_buffer));
                res = VTL_sub_ResizeSubEntryArray(arr_ptr, arr_cap_ptr, *arr_len_ptr);
                if (res != VTL_res_kOk) return res;
                (*arr_ptr)[*arr_len_ptr].index = current_vtt_index++;
                (*arr_ptr)[*arr_len_ptr].start = start;
                (*arr_ptr)[*arr_len_ptr].end = end;
                (*arr_ptr)[*arr_len_ptr].text = strdup(text_buffer);
                if (!(*arr_ptr)[*arr_len_ptr].text) return VTL_res_kAllocError;
                (*arr_ptr)[*arr_len_ptr].style = NULL;
                (*arr_len_ptr)++;
            }
        }
    }
    return VTL_res_kOk;
}

static VTL_AppResult VTL_sub_ParseFileASS(FILE* f, VTL_sub_Entry** arr_ptr, size_t* arr_cap_ptr, size_t* arr_len_ptr) {
    char line_buffer[1024];
    VTL_AppResult res = VTL_res_kOk;
    int found_events = 0;
    while (fgets(line_buffer, sizeof(line_buffer), f) && !found_events) {
        if (strstr(line_buffer, "[Events]")) found_events = 1;
    }
    if (!fgets(line_buffer, sizeof(line_buffer), f)) return VTL_res_kSubtitleFormatError;
    int current_ass_index = 1;
    while (fgets(line_buffer, sizeof(line_buffer), f)) {
        int skip = 0;
        if (strncmp(line_buffer, "Dialogue:", 9) != 0) skip = 1;
        if (!skip) {
            char* p_line = line_buffer + 9;
            char* fields[10] = {0};
            int field_count = 0;
            char* token = p_line;
            for (int i = 0; i < 9 && token; ++i) {
                fields[field_count++] = token;
                char* comma = strchr(token, ',');
                if (comma) {
                    *comma = '\0';
                    token = comma + 1;
                } else {
                    token = NULL;
                }
            }
            if (token) {
                fields[field_count++] = token;
                char* nl = strchr(token, '\n'); if (nl) *nl = '\0';
                nl = strchr(token, '\r'); if (nl) *nl = '\0';
            }
            if (field_count < 10) skip = 1;
            if (!skip && (!fields[1] || !fields[2] || !fields[3] || !fields[9])) skip = 1;
            if (!skip) {
                double start, end;
                res = VTL_sub_ParseTime(fields[1], VTL_sub_format_kASS, &start);
                if (res != VTL_res_kOk) return res;
                res = VTL_sub_ParseTime(fields[2], VTL_sub_format_kASS, &end);
                if (res != VTL_res_kOk) return res;
                res = VTL_sub_ResizeSubEntryArray(arr_ptr, arr_cap_ptr, *arr_len_ptr);
                if (res != VTL_res_kOk) return res;
                (*arr_ptr)[*arr_len_ptr].index = current_ass_index++;
                (*arr_ptr)[*arr_len_ptr].start = start;
                (*arr_ptr)[*arr_len_ptr].end = end;
                (*arr_ptr)[*arr_len_ptr].text = strdup(fields[9]);
                (*arr_ptr)[*arr_len_ptr].style = strdup(fields[3]);
                if (!(*arr_ptr)[*arr_len_ptr].text || !(*arr_ptr)[*arr_len_ptr].style) {
                    free((*arr_ptr)[*arr_len_ptr].text);
                    free((*arr_ptr)[*arr_len_ptr].style);
                    (*arr_ptr)[*arr_len_ptr].text = NULL;
                    (*arr_ptr)[*arr_len_ptr].style = NULL;
                    return VTL_res_kAllocError;
                }
                (*arr_len_ptr)++;
            }
        }
    }
    return VTL_res_kOk;
}