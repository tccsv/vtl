#include <VTL/VTL.h>
#include <VTL/VTL_app_result.h>
#include <VTL/publication/text/VTL_publication_text_op.h>
#include <VTL/publication/text/infra/VTL_publication_text_read.h>
#include <VTL/publication/text/infra/VTL_publication_text_write.h>
#include <vtl_tests/VTL_test_data.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Функция для проверки равенства строк
static int VTL_test_AssertStringsEqual(const char* actual, const char* expected) {
    if (strcmp(actual, expected) != 0) {
        printf("Ошибка: ожидалось '%s', получено '%s'\n", expected, actual);
        return 0;
    }
    return 1;
}

// Тест 1: Преобразование простого HTML в размеченный текст
static int VTL_test_HtmlToMarkedTextSimple() {
    // Создаем простой HTML текст
    const char* html_str = "<p>Это <b>жирный</b> текст</p>";
    VTL_publication_Text html_text;
    html_text.text = (VTL_publication_text_Symbol*)html_str;
    html_text.length = strlen(html_str);
    
    // Преобразуем в размеченный текст
    VTL_publication_MarkedText* marked_text = NULL;
    VTL_AppResult result = VTL_publication_text_InitFromHTML(&marked_text, &html_text);
    
    // Проверяем результат
    const char* test_fail_message = "\nОшибка преобразования HTML в размеченный текст\n";
    if (!VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message)) {
        return 0;
    }
    
    // Проверяем, что размеченный текст создан
    test_fail_message = "\nОшибка: размеченный текст не создан\n";
    if (!VTL_test_CheckCondition(marked_text && marked_text->parts, test_fail_message)) {
        return 0;
    }
    
    // Проверяем, что есть хотя бы одна жирная часть
    int has_bold_part = 0;
    for (size_t i = 0; i < marked_text->length; i++) {
        if (marked_text->parts[i].type & VTL_TEXT_MODIFICATION_BOLD) {
            has_bold_part = 1;
            break;
        }
    }
    
    test_fail_message = "\nОшибка: не найдена жирная часть текста\n";
    if (!VTL_test_CheckCondition(has_bold_part, test_fail_message)) {
        VTL_publication_marked_text_Free(marked_text);
        return 0;
    }
    
    VTL_publication_marked_text_Free(marked_text);
    return 1;
}

// Тест 2: Преобразование размеченного текста в HTML
static int VTL_test_MarkedTextToHtml() {
    // Создаем размеченный текст вручную с ASCII символами
    VTL_publication_MarkedText marked_text;
    marked_text.length = 3;
    marked_text.parts = (VTL_publication_marked_text_Part*)malloc(3 * sizeof(VTL_publication_marked_text_Part));
    
    // Часть 1: обычный текст "This is "
    marked_text.parts[0].text = strdup("This is ");
    marked_text.parts[0].length = 8;
    marked_text.parts[0].type = 0;
    
    // Часть 2: жирный текст "bold"
    marked_text.parts[1].text = strdup("bold");
    marked_text.parts[1].length = 4;
    marked_text.parts[1].type = VTL_TEXT_MODIFICATION_BOLD;
    
    // Часть 3: обычный текст " text"
    marked_text.parts[2].text = strdup(" text");
    marked_text.parts[2].length = 5;
    marked_text.parts[2].type = 0;
    
    // Преобразуем в HTML
    VTL_publication_Text* html_text = NULL;
    VTL_AppResult result = VTL_publication_marked_text_TransformToHTML(&html_text, &marked_text);
    
    // Проверяем результат
    const char* test_fail_message = "\nОшибка преобразования размеченного текста в HTML\n";
    if (!VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message)) {
        // Освобождаем память вручную, так как не можем использовать Free для стековой переменной
        for (size_t i = 0; i < marked_text.length; i++) {
            free(marked_text.parts[i].text);
        }
        free(marked_text.parts);
        return 0;
    }
    
    // Проверяем, что HTML содержит нужные теги и части текста
    test_fail_message = "\nОшибка: HTML не содержит необходимых элементов\n";
    if (!VTL_test_CheckCondition(
        strstr(html_text->text, "<b>") != NULL && 
        strstr(html_text->text, "</b>") != NULL &&
        strstr(html_text->text, "This is") != NULL &&
        strstr(html_text->text, "bold") != NULL &&
        strstr(html_text->text, "text") != NULL, 
        test_fail_message)) {
        
        VTL_publication_text_Free(html_text);
        for (size_t i = 0; i < marked_text.length; i++) {
            free(marked_text.parts[i].text);
        }
        free(marked_text.parts);
        return 0;
    }
    
    // Освобождаем память
    VTL_publication_text_Free(html_text);
    
    // Освобождаем память вручную, так как не можем использовать Free для стековой переменной
    for (size_t i = 0; i < marked_text.length; i++) {
        free(marked_text.parts[i].text);
    }
    free(marked_text.parts);
    
    return 1;
}

// Тест 3: Преобразование размеченного текста в обычный текст
static int VTL_test_MarkedTextToRegularText() {
    // Создаем размеченный текст вручную с ASCII символами
    VTL_publication_MarkedText marked_text;
    marked_text.length = 3;
    marked_text.parts = (VTL_publication_marked_text_Part*)malloc(3 * sizeof(VTL_publication_marked_text_Part));
    
    // Часть 1: обычный текст "This is "
    marked_text.parts[0].text = strdup("This is ");
    marked_text.parts[0].length = 8;
    marked_text.parts[0].type = 0;
    
    // Часть 2: жирный текст "bold"
    marked_text.parts[1].text = strdup("bold");
    marked_text.parts[1].length = 4;
    marked_text.parts[1].type = VTL_TEXT_MODIFICATION_BOLD;
    
    // Часть 3: обычный текст " text"
    marked_text.parts[2].text = strdup(" text");
    marked_text.parts[2].length = 5;
    marked_text.parts[2].type = 0;
    
    // Преобразуем в обычный текст
    VTL_publication_Text* regular_text = NULL;
    VTL_AppResult result = VTL_publication_marked_text_TransformToRegularText(&regular_text, &marked_text);
    
    // Проверяем результат
    const char* test_fail_message = "\nОшибка преобразования размеченного текста в обычный текст\n";
    if (!VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message)) {
        // Освобождаем память вручную
        for (size_t i = 0; i < marked_text.length; i++) {
            free(marked_text.parts[i].text);
        }
        free(marked_text.parts);
        return 0;
    }
    
    // Проверяем, что текст содержит все нужные части
    test_fail_message = "\nОшибка: текст не содержит необходимых элементов\n";
    if (!VTL_test_CheckCondition(
        strstr(regular_text->text, "This is") != NULL &&
        strstr(regular_text->text, "bold") != NULL &&
        strstr(regular_text->text, "text") != NULL,
        test_fail_message)) {
        
        VTL_publication_text_Free(regular_text);
        for (size_t i = 0; i < marked_text.length; i++) {
            free(marked_text.parts[i].text);
        }
        free(marked_text.parts);
        return 0;
    }
    
    // Освобождаем память
    VTL_publication_text_Free(regular_text);
    
    // Освобождаем память вручную
    for (size_t i = 0; i < marked_text.length; i++) {
        free(marked_text.parts[i].text);
    }
    free(marked_text.parts);
    
    return 1;
}

// Тест 4: Проверка обработки различных HTML тегов
static int VTL_test_HtmlTagsProcessing() {
    // Создаем HTML с разными тегами
    const char* html_str = "<p>Это <b>жирный</b>, <i>курсивный</i> и <s>зачеркнутый</s> текст</p>";
    VTL_publication_Text html_text;
    html_text.text = (VTL_publication_text_Symbol*)html_str;
    html_text.length = strlen(html_str);
    
    // Преобразуем в размеченный текст
    VTL_publication_MarkedText* marked_text = NULL;
    VTL_AppResult result = VTL_publication_text_InitFromHTML(&marked_text, &html_text);
    
    // Проверяем результат
    const char* test_fail_message = "\nОшибка преобразования HTML в размеченный текст\n";
    if (!VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message)) {
        return 0;
    }
    
    // Преобразуем обратно в HTML
    VTL_publication_Text* html_result = NULL;
    result = VTL_publication_marked_text_TransformToHTML(&html_result, marked_text);
    
    test_fail_message = "\nОшибка преобразования размеченного текста в HTML\n";
    if (!VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message)) {
        VTL_publication_marked_text_Free(marked_text);
        return 0;
    }
    
    // Ожидаемый HTML может немного отличаться, но должен содержать основные теги
    test_fail_message = "\nОшибка: не все теги присутствуют в результате\n";
    if (!VTL_test_CheckCondition(
        strstr(html_result->text, "<b>") != NULL && 
        strstr(html_result->text, "</b>") != NULL &&
        strstr(html_result->text, "<i>") != NULL && 
        strstr(html_result->text, "</i>") != NULL &&
        strstr(html_result->text, "<s>") != NULL && 
        strstr(html_result->text, "</s>") != NULL,
        test_fail_message)) {
        
        VTL_publication_marked_text_Free(marked_text);
        VTL_publication_text_Free(html_result);
        return 0;
    }
    
    // Освобождаем память
    VTL_publication_marked_text_Free(marked_text);
    VTL_publication_text_Free(html_result);
    
    return 1;
}

// Тест 5: Обработка вложенных HTML тегов
static int VTL_test_NestedHtmlTags() {
    // Создаем HTML с вложенными тегами
    const char* html_str = "<p>Это <b>жирный <i>и курсивный</i></b> текст</p>";
    VTL_publication_Text html_text;
    html_text.text = (VTL_publication_text_Symbol*)html_str;
    html_text.length = strlen(html_str);
    
    // Преобразуем в размеченный текст
    VTL_publication_MarkedText* marked_text = NULL;
    VTL_AppResult result = VTL_publication_text_InitFromHTML(&marked_text, &html_text);
    
    // Проверяем результат
    const char* test_fail_message = "\nОшибка преобразования HTML с вложенными тегами в размеченный текст\n";
    if (!VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message)) {
        return 0;
    }
    
    // Проверяем, что есть часть, которая одновременно жирная и курсивная
    int has_bold_and_italic_part = 0;
    for (size_t i = 0; i < marked_text->length; i++) {
        if ((marked_text->parts[i].type & VTL_TEXT_MODIFICATION_BOLD) && 
            (marked_text->parts[i].type & VTL_TEXT_MODIFICATION_ITALIC)) {
            has_bold_and_italic_part = 1;
            break;
        }
    }
    
    test_fail_message = "\nОшибка: не найдена часть текста, которая одновременно жирная и курсивная\n";
    if (!VTL_test_CheckCondition(has_bold_and_italic_part, test_fail_message)) {
        VTL_publication_marked_text_Free(marked_text);
        return 0;
    }
    
    VTL_publication_marked_text_Free(marked_text);
    return 1;
}

// Тест 6: Обработка HTML с переносами строк
static int VTL_test_HtmlWithLineBreaks() {
    // Создаем HTML с переносами строк
    const char* html_str = "<p>Строка 1<br>Строка 2</p>";
    VTL_publication_Text html_text;
    html_text.text = (VTL_publication_text_Symbol*)html_str;
    html_text.length = strlen(html_str);
    
    // Преобразуем в размеченный текст
    VTL_publication_MarkedText* marked_text = NULL;
    VTL_AppResult result = VTL_publication_text_InitFromHTML(&marked_text, &html_text);
    
    // Проверяем результат
    const char* test_fail_message = "\nОшибка преобразования HTML с переносами строк в размеченный текст\n";
    if (!VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message)) {
        return 0;
    }
    
    // Преобразуем в обычный текст для проверки переносов строк
    VTL_publication_Text* regular_text = NULL;
    result = VTL_publication_marked_text_TransformToRegularText(&regular_text, marked_text);
    
    test_fail_message = "\nОшибка преобразования размеченного текста с переносами строк в обычный текст\n";
    if (!VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message)) {
        VTL_publication_marked_text_Free(marked_text);
        return 0;
    }
    
    // Проверяем, что текст содержит перенос строки
    test_fail_message = "\nОшибка: текст не содержит переноса строки\n";
    if (!VTL_test_CheckCondition(strchr(regular_text->text, '\n') != NULL, test_fail_message)) {
        VTL_publication_text_Free(regular_text);
        VTL_publication_marked_text_Free(marked_text);
        return 0;
    }
    
    // Освобождаем память
    VTL_publication_text_Free(regular_text);
    VTL_publication_marked_text_Free(marked_text);
    
    return 1;
}

// Тест 7: Обработка пустого HTML
static int VTL_test_EmptyHtml() {
    // Создаем пустой HTML
    const char* html_str = "";
    VTL_publication_Text html_text;
    html_text.text = (VTL_publication_text_Symbol*)html_str;
    html_text.length = 0;
    
    // Преобразуем в размеченный текст
    VTL_publication_MarkedText* marked_text = NULL;
    VTL_AppResult result = VTL_publication_text_InitFromHTML(&marked_text, &html_text);
    
    // Проверяем результат
    const char* test_fail_message = "\nОшибка преобразования пустого HTML в размеченный текст\n";
    if (!VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message)) {
        return 0;
    }
    
    // Проверяем, что результат пуст или содержит 0 частей
    test_fail_message = "\nОшибка: размеченный текст должен быть пустым\n";
    if (!VTL_test_CheckCondition(marked_text->length == 0, test_fail_message)) {
        VTL_publication_marked_text_Free(marked_text);
        return 0;
    }
    
    VTL_publication_marked_text_Free(marked_text);
    return 1;
}

// Тест 8: Обработка HTML с различными типами тегов <strong>, <em>, <del>
static int VTL_test_HtmlWithVariousTags() {
    // Создаем HTML с различными типами тегов
    const char* html_str = "<p><strong>Жирный</strong> <em>курсивный</em> <del>зачеркнутый</del></p>";
    VTL_publication_Text html_text;
    html_text.text = (VTL_publication_text_Symbol*)html_str;
    html_text.length = strlen(html_str);
    
    // Преобразуем в размеченный текст
    VTL_publication_MarkedText* marked_text = NULL;
    VTL_AppResult result = VTL_publication_text_InitFromHTML(&marked_text, &html_text);
    
    // Проверяем результат
    const char* test_fail_message = "\nОшибка преобразования HTML с различными типами тегов в размеченный текст\n";
    if (!VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message)) {
        return 0;
    }
    
    // Проверяем, что есть хотя бы одна жирная часть
    int has_bold_part = 0;
    // Проверяем, что есть хотя бы одна курсивная часть
    int has_italic_part = 0;
    // Проверяем, что есть хотя бы одна зачеркнутая часть
    int has_strike_part = 0;
    
    for (size_t i = 0; i < marked_text->length; i++) {
        if (marked_text->parts[i].type & VTL_TEXT_MODIFICATION_BOLD) {
            has_bold_part = 1;
        }
        if (marked_text->parts[i].type & VTL_TEXT_MODIFICATION_ITALIC) {
            has_italic_part = 1;
        }
        if (marked_text->parts[i].type & VTL_TEXT_MODIFICATION_STRIKETHROUGH) {
            has_strike_part = 1;
        }
    }
    
    // Проверяем наличие всех типов форматирования
    test_fail_message = "\nОшибка: не найдена жирная часть текста\n";
    if (!VTL_test_CheckCondition(has_bold_part, test_fail_message)) {
        VTL_publication_marked_text_Free(marked_text);
        return 0;
    }
    
    test_fail_message = "\nОшибка: не найдена курсивная часть текста\n";
    if (!VTL_test_CheckCondition(has_italic_part, test_fail_message)) {
        VTL_publication_marked_text_Free(marked_text);
        return 0;
    }
    
    test_fail_message = "\nОшибка: не найдена зачеркнутая часть текста\n";
    if (!VTL_test_CheckCondition(has_strike_part, test_fail_message)) {
        VTL_publication_marked_text_Free(marked_text);
        return 0;
    }
    
    VTL_publication_marked_text_Free(marked_text);
    return 1;
}

// Главная функция с запуском тестов
int main(void)
{
    // Запускаем все тесты и считаем количество ошибок
    int fail_count = 0;
    
    // Для каждого теста увеличиваем счетчик, если тест не пройден
    if (!VTL_test_HtmlToMarkedTextSimple()) fail_count++;
    if (!VTL_test_MarkedTextToHtml()) fail_count++;
    if (!VTL_test_MarkedTextToRegularText()) fail_count++;
    if (!VTL_test_HtmlTagsProcessing()) fail_count++;
    
    // Новые тесты
    if (!VTL_test_NestedHtmlTags()) fail_count++;
    if (!VTL_test_HtmlWithLineBreaks()) fail_count++;
    if (!VTL_test_EmptyHtml()) fail_count++;
    if (!VTL_test_HtmlWithVariousTags()) fail_count++;
    
    // Возвращаем число ошибок
    return fail_count;
} 