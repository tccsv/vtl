#include <VTL/publication/text/VTL_publication_text_op.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Функция для сравнения строк без учета регистра (замена strncasecmp)
static int VTL_publication_text_StringCompareInsensitive(const char* s1, const char* s2, size_t n) {
    if (s1 == s2) {
        return 0;
    }
    if (s1 == NULL) {
        return -1;
    }
    if (s2 == NULL) {
        return 1;
    }
    
    for (size_t i = 0; i < n; i++) {
        int c1 = tolower((unsigned char)s1[i]);
        int c2 = tolower((unsigned char)s2[i]);
        if (c1 != c2) {
            return c1 - c2;
        }
        if (c1 == '\0') {
            return 0;
        }
    }
    return 0;
}

// Функция для освобождения памяти размеченного текста
VTL_AppResult VTL_publication_marked_text_Free(VTL_publication_MarkedText* p_marked_text) {
    if (!p_marked_text) {
        return VTL_res_kInvalidParamErr;
    }
    
    // Освобождаем память каждой части
    for (size_t i = 0; i < p_marked_text->length; i++) {
        free(p_marked_text->parts[i].text);
    }
    
    // Освобождаем массив частей
    free(p_marked_text->parts);
    
    // Освобождаем структуру
    free(p_marked_text);
    
    return VTL_res_kOk;
}

// Функция для освобождения памяти обычного текста
VTL_AppResult VTL_publication_text_Free(VTL_publication_Text* p_text) {
    if (!p_text) {
        return VTL_res_kInvalidParamErr;
    }
    
    // Освобождаем текст
    free(p_text->text);
    
    // Освобождаем структуру
    free(p_text);
    
    return VTL_res_kOk;
}

static VTL_AppResult VTL_publication_text_InitFromStandartMD(VTL_publication_MarkedText** pp_publication, 
                                                       const VTL_publication_Text* p_src_text)
{
    return VTL_res_kOk;
}

static VTL_AppResult VTL_publication_text_InitFromTelegramMD(VTL_publication_MarkedText** pp_publication, 
                                                        const VTL_publication_Text* p_src_text)
{
    return VTL_res_kOk;
}

VTL_AppResult VTL_publication_text_InitFromHTML(VTL_publication_MarkedText** pp_publication, 
                                                    const VTL_publication_Text* p_src_text)
{
    if (!pp_publication || !p_src_text || !p_src_text->text) {
        return VTL_res_kInvalidParamErr;
    }
    
    // Выделяем память для размеченного текста
    *pp_publication = (VTL_publication_MarkedText*)malloc(sizeof(VTL_publication_MarkedText));
    if (!*pp_publication) {
        return VTL_res_kMemAllocErr;
    }
    
    // Инициализируем начальную емкость для частей
    size_t capacity = 16;
    (*pp_publication)->parts = (VTL_publication_marked_text_Part*)malloc(capacity * sizeof(VTL_publication_marked_text_Part));
    if (!(*pp_publication)->parts) {
        free(*pp_publication);
        return VTL_res_kMemAllocErr;
    }
    (*pp_publication)->length = 0;
    
    const VTL_publication_text_Symbol* text = p_src_text->text;
    size_t text_len = p_src_text->length;
    size_t pos = 0;
    
    // Состояния разметки
    int in_bold = 0;
    int in_italic = 0;
    int in_strike = 0;
    
    // Временный буфер для хранения текста между тегами
    VTL_publication_text_Symbol* buffer = (VTL_publication_text_Symbol*)malloc((text_len + 1) * sizeof(VTL_publication_text_Symbol));
    if (!buffer) {
        free((*pp_publication)->parts);
        free(*pp_publication);
        return VTL_res_kMemAllocErr;
    }
    
    size_t buffer_pos = 0;
    
    // Функция для добавления части текста в размеченный текст
    void VTL_publication_text_AddTextPart() {
        if (buffer_pos > 0) {
            // Проверяем, нужно ли увеличить емкость
            if ((*pp_publication)->length >= capacity) {
                capacity *= 2;
                VTL_publication_marked_text_Part* new_parts = (VTL_publication_marked_text_Part*)realloc(
                    (*pp_publication)->parts, capacity * sizeof(VTL_publication_marked_text_Part));
                if (!new_parts) {
                    free(buffer);
                    free((*pp_publication)->parts);
                    free(*pp_publication);
                    return;
                }
                (*pp_publication)->parts = new_parts;
            }
            
            // Создаем новую часть
            VTL_publication_marked_text_Part* part = &(*pp_publication)->parts[(*pp_publication)->length++];
            
            // Выделяем память для текста части и добавляем завершающий нуль
            part->text = (VTL_publication_text_Symbol*)malloc((buffer_pos + 1) * sizeof(VTL_publication_text_Symbol));
            if (!part->text) {
                free(buffer);
                // Освобождаем память предыдущих частей
                for (size_t i = 0; i < (*pp_publication)->length - 1; i++) {
                    free((*pp_publication)->parts[i].text);
                }
                free((*pp_publication)->parts);
                free(*pp_publication);
                return;
            }
            
            // Копируем текст
            memcpy(part->text, buffer, buffer_pos * sizeof(VTL_publication_text_Symbol));
            part->text[buffer_pos] = '\0';
            part->length = buffer_pos;
            
            // Устанавливаем тип форматирования
            part->type = 0;
            if (in_bold) VTL_publication_marked_text_modification_SetBold(&part->type);
            if (in_italic) VTL_publication_marked_text_modification_SetItalic(&part->type);
            if (in_strike) VTL_publication_marked_text_modification_SetStrikethrough(&part->type);
            
            // Сбрасываем буфер
            buffer_pos = 0;
        }
    };
    
    while (pos < text_len) {
        if (text[pos] == '<') {
            // Нашли открывающий тег
            size_t tag_start = pos;
            pos++;
            
            // Пропускаем пробелы после <
            while (pos < text_len && isspace(text[pos])) pos++;
            
            // Проверяем, закрывающий ли это тег
            int is_closing = 0;
            if (pos < text_len && text[pos] == '/') {
                is_closing = 1;
                pos++;
                // Пропускаем пробелы после /
                while (pos < text_len && isspace(text[pos])) pos++;
            }
            
            // Читаем имя тега
            size_t tag_name_start = pos;
            while (pos < text_len && text[pos] != '>' && !isspace(text[pos])) pos++;
            size_t tag_name_end = pos;
            
            // Пропускаем атрибуты и находим закрывающую >
            while (pos < text_len && text[pos] != '>') pos++;
            
            if (pos < text_len && text[pos] == '>') {
                // Закрываем текущую часть текста перед тегом
                VTL_publication_text_AddTextPart();
                
                // Проверяем, какой тег мы нашли
                size_t tag_len = tag_name_end - tag_name_start;
                if (tag_len == 1 && (text[tag_name_start] == 'b' || text[tag_name_start] == 'B')) {
                    // Тег <b> или </b>
                    in_bold = !is_closing;
                } else if (tag_len == 6 && VTL_publication_text_StringCompareInsensitive(&text[tag_name_start], "strong", 6) == 0) {
                    // Тег <strong> или </strong>
                    in_bold = !is_closing;
                } else if (tag_len == 1 && (text[tag_name_start] == 'i' || text[tag_name_start] == 'I')) {
                    // Тег <i> или </i>
                    in_italic = !is_closing;
                } else if (tag_len == 2 && VTL_publication_text_StringCompareInsensitive(&text[tag_name_start], "em", 2) == 0) {
                    // Тег <em> или </em>
                    in_italic = !is_closing;
                } else if (tag_len == 1 && (text[tag_name_start] == 's' || text[tag_name_start] == 'S')) {
                    // Тег <s> или </s>
                    in_strike = !is_closing;
                } else if (tag_len == 3 && VTL_publication_text_StringCompareInsensitive(&text[tag_name_start], "del", 3) == 0) {
                    // Тег <del> или </del>
                    in_strike = !is_closing;
                } else if (tag_len == 6 && VTL_publication_text_StringCompareInsensitive(&text[tag_name_start], "strike", 6) == 0) {
                    // Тег <strike> или </strike>
                    in_strike = !is_closing;
                } else if (tag_len == 2 && VTL_publication_text_StringCompareInsensitive(&text[tag_name_start], "br", 2) == 0) {
                    // Тег <br> - добавляем перенос строки в буфер
                    buffer[buffer_pos++] = '\n';
                } else if (tag_len == 1 && (text[tag_name_start] == 'p' || text[tag_name_start] == 'P')) {
                    // Теги <p> и </p> - добавляем перенос строки
                    if (is_closing) {
                        buffer[buffer_pos++] = '\n';
                        buffer[buffer_pos++] = '\n';
                    }
                }
                
                pos++; // Пропускаем >
            }
        } else {
            // Обычный текст - добавляем в буфер
            buffer[buffer_pos++] = text[pos++];
        }
    }
    
    // Добавляем оставшийся текст в буфере
    VTL_publication_text_AddTextPart();
    
    free(buffer);
    return VTL_res_kOk;
}

static VTL_AppResult VTL_publication_text_InitFromBB(VTL_publication_MarkedText** pp_publication, 
                                                const VTL_publication_Text* p_src_text)
{
    return VTL_res_kOk;
}

VTL_AppResult VTL_publication_marked_text_Init(VTL_publication_MarkedText **pp_marked_text, 
                                                const VTL_publication_Text *p_src_text, 
                                                const VTL_publication_marked_text_MarkupType src_markup_type)
{
    if (!pp_marked_text || !p_src_text) {
        return VTL_res_kInvalidParamErr;
    }
    
    if(src_markup_type == VTL_markup_type_kStandartMD)
    {        
        return VTL_publication_text_InitFromStandartMD(pp_marked_text, p_src_text);
    }
    if(src_markup_type == VTL_markup_type_kTelegramMD)
    {
        return VTL_publication_text_InitFromTelegramMD(pp_marked_text, p_src_text);
    }
    if(src_markup_type == VTL_markup_type_kHTML)
    {
        return VTL_publication_text_InitFromHTML(pp_marked_text, p_src_text);
    }
    if(src_markup_type == VTL_markup_type_kBB)
    {
        return VTL_publication_text_InitFromBB(pp_marked_text, p_src_text);
    }
    return VTL_res_kInvalidParamErr;
}

VTL_AppResult VTL_publication_marked_text_TransformToRegularText(VTL_publication_Text** pp_out_marked_text,
                                                    const VTL_publication_MarkedText* p_src_marked_text)
{
    if (!pp_out_marked_text || !p_src_marked_text) {
        return VTL_res_kInvalidParamErr;
    }
    
    // Подсчитываем общий размер текста
    size_t total_size = 0;
    for (size_t i = 0; i < p_src_marked_text->length; i++) {
        total_size += p_src_marked_text->parts[i].length;
    }
    
    // Выделяем память для результата
    *pp_out_marked_text = (VTL_publication_Text*)malloc(sizeof(VTL_publication_Text));
    if (!*pp_out_marked_text) {
        return VTL_res_kMemAllocErr;
    }
    
    (*pp_out_marked_text)->text = (VTL_publication_text_Symbol*)malloc((total_size + 1) * sizeof(VTL_publication_text_Symbol));
    if (!(*pp_out_marked_text)->text) {
        free(*pp_out_marked_text);
        return VTL_res_kMemAllocErr;
    }
    
    // Копируем текст из всех частей в результат
    size_t pos = 0;
    for (size_t i = 0; i < p_src_marked_text->length; i++) {
        if (p_src_marked_text->parts[i].length > 0 && p_src_marked_text->parts[i].text) {
            memcpy(&(*pp_out_marked_text)->text[pos], p_src_marked_text->parts[i].text, 
                   p_src_marked_text->parts[i].length * sizeof(VTL_publication_text_Symbol));
            pos += p_src_marked_text->parts[i].length;
        }
    }
    
    (*pp_out_marked_text)->text[pos] = '\0';
    (*pp_out_marked_text)->length = pos;
    
    return VTL_res_kOk;
}

VTL_AppResult VTL_publication_marked_text_TransformToStandartMD(VTL_publication_Text** pp_out_marked_text,
                                                    const VTL_publication_MarkedText* p_src_marked_text)
{
    return VTL_res_kOk;
}

VTL_AppResult VTL_publication_marked_text_TransformToTelegramMD(VTL_publication_Text** pp_out_marked_text,
                                                    const VTL_publication_MarkedText* p_src_marked_text)
{
    return VTL_res_kOk;
}

VTL_AppResult VTL_publication_marked_text_TransformToHTML(VTL_publication_Text** pp_out_marked_text,
                                                    const VTL_publication_MarkedText* p_src_marked_text)
{
    if (!pp_out_marked_text || !p_src_marked_text) {
        return VTL_res_kInvalidParamErr;
    }
    
    // Оценка размера выходного HTML
    size_t estimated_size = 0;
    for (size_t i = 0; i < p_src_marked_text->length; i++) {
        VTL_publication_marked_text_Part* part = &p_src_marked_text->parts[i];
        estimated_size += part->length;
        
        // Добавляем размер для тегов форматирования
        if (part->type & VTL_TEXT_MODIFICATION_BOLD) {
            estimated_size += 7; // <b></b>
        }
        if (part->type & VTL_TEXT_MODIFICATION_ITALIC) {
            estimated_size += 7; // <i></i>
        }
        if (part->type & VTL_TEXT_MODIFICATION_STRIKETHROUGH) {
            estimated_size += 7; // <s></s>
        }
    }
    
    // Выделяем память для HTML
    *pp_out_marked_text = (VTL_publication_Text*)malloc(sizeof(VTL_publication_Text));
    if (!*pp_out_marked_text) {
        return VTL_res_kMemAllocErr;
    }
    
    (*pp_out_marked_text)->text = (VTL_publication_text_Symbol*)malloc((estimated_size + 1) * sizeof(VTL_publication_text_Symbol));
    if (!(*pp_out_marked_text)->text) {
        free(*pp_out_marked_text);
        return VTL_res_kMemAllocErr;
    }
    
    // Формируем HTML
    char* html = (char*)(*pp_out_marked_text)->text;
    size_t pos = 0;
    
    for (size_t i = 0; i < p_src_marked_text->length; i++) {
        VTL_publication_marked_text_Part* part = &p_src_marked_text->parts[i];
        
        if (part->length > 0 && part->text) {
            // Открывающие теги
            if (part->type & VTL_TEXT_MODIFICATION_BOLD) {
                strcpy(&html[pos], "<b>");
                pos += 3;
            }
            if (part->type & VTL_TEXT_MODIFICATION_ITALIC) {
                strcpy(&html[pos], "<i>");
                pos += 3;
            }
            if (part->type & VTL_TEXT_MODIFICATION_STRIKETHROUGH) {
                strcpy(&html[pos], "<s>");
                pos += 3;
            }
            
            // Копируем текст
            memcpy(&html[pos], part->text, part->length);
            pos += part->length;
            
            // Закрывающие теги (в обратном порядке)
            if (part->type & VTL_TEXT_MODIFICATION_STRIKETHROUGH) {
                strcpy(&html[pos], "</s>");
                pos += 4;
            }
            if (part->type & VTL_TEXT_MODIFICATION_ITALIC) {
                strcpy(&html[pos], "</i>");
                pos += 4;
            }
            if (part->type & VTL_TEXT_MODIFICATION_BOLD) {
                strcpy(&html[pos], "</b>");
                pos += 4;
            }
        }
    }
    
    html[pos] = '\0';
    (*pp_out_marked_text)->length = pos;
    
    return VTL_res_kOk;
}

VTL_AppResult VTL_publication_marked_text_TransformToBB(VTL_publication_Text** pp_out_marked_text,
                                                    const VTL_publication_MarkedText* p_src_marked_text)
{
    return VTL_res_kOk;
}