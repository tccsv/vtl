#include <VTL/VTL.h>
#include <VTL/VTL_app_result.h>
#include <VTL/publication/text/VTL_publication_text_op.h>
#include <VTL/publication/text/infra/VTL_publication_text_read.h>
#include <VTL/publication/text/infra/VTL_publication_text_write.h>
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
    printf("Тест 1: Преобразование простого HTML в размеченный текст\n");
    
    // Создаем простой HTML текст
    const char* html_str = "<p>Это <b>жирный</b> текст</p>";
    VTL_publication_Text html_text;
    html_text.text = (VTL_publication_text_Symbol*)html_str;
    html_text.length = strlen(html_str);
    
    // Преобразуем в размеченный текст
    VTL_publication_MarkedText* marked_text = NULL;
    VTL_AppResult result = VTL_publication_text_InitFromHTML(&marked_text, &html_text);
    
    // Проверяем результат
    if (result != VTL_res_kOk) {
        printf("Ошибка преобразования HTML в размеченный текст: %d\n", result);
        return 0;
    }
    
    // Проверяем, что размеченный текст создан
    if (!marked_text || !marked_text->parts) {
        printf("Ошибка: размеченный текст не создан\n");
        return 0;
    }
    
    // Выводим информацию о частях для отладки
    printf("Количество частей: %zu\n", marked_text->length);
    for (size_t i = 0; i < marked_text->length; i++) {
        printf("Часть %zu: длина=%zu, тип=%d, текст='", i + 1, marked_text->parts[i].length, marked_text->parts[i].type);
        for (size_t j = 0; j < marked_text->parts[i].length; j++) {
            printf("%c", marked_text->parts[i].text[j]);
        }
        printf("'\n");
    }
    
    // Проверяем, что есть хотя бы одна жирная часть
    int has_bold_part = 0;
    for (size_t i = 0; i < marked_text->length; i++) {
        if (marked_text->parts[i].type & VTL_TEXT_MODIFICATION_BOLD) {
            has_bold_part = 1;
            break;
        }
    }
    
    if (!has_bold_part) {
        printf("Ошибка: не найдена жирная часть текста\n");
        VTL_publication_marked_text_Free(marked_text);
        return 0;
    }
    
    VTL_publication_marked_text_Free(marked_text);
    printf("Тест 1 пройден успешно\n");
    return 1;
}

// Тест 2: Преобразование размеченного текста в HTML
static int VTL_test_MarkedTextToHtml() {
    printf("Тест 2: Преобразование размеченного текста в HTML\n");
    
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
    if (result != VTL_res_kOk) {
        printf("Ошибка преобразования размеченного текста в HTML: %d\n", result);
        // Освобождаем память вручную, так как не можем использовать Free для стековой переменной
        for (size_t i = 0; i < marked_text.length; i++) {
            free(marked_text.parts[i].text);
        }
        free(marked_text.parts);
        return 0;
    }
    
    // Выводим полученный HTML для отладки
    printf("Полученный HTML: '%s'\n", html_text->text);
    
    // Проверяем, что HTML содержит нужные теги и части текста
    if (strstr(html_text->text, "<b>") == NULL || 
        strstr(html_text->text, "</b>") == NULL ||
        strstr(html_text->text, "This is") == NULL ||
        strstr(html_text->text, "bold") == NULL ||
        strstr(html_text->text, "text") == NULL) {
        printf("Ошибка: HTML не содержит необходимых элементов\n");
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
    
    printf("Тест 2 пройден успешно\n");
    return 1;
}

// Тест 3: Преобразование размеченного текста в обычный текст
static int VTL_test_MarkedTextToRegularText() {
    printf("Тест 3: Преобразование размеченного текста в обычный текст\n");
    
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
    if (result != VTL_res_kOk) {
        printf("Ошибка преобразования размеченного текста в обычный текст: %d\n", result);
        // Освобождаем память вручную
        for (size_t i = 0; i < marked_text.length; i++) {
            free(marked_text.parts[i].text);
        }
        free(marked_text.parts);
        return 0;
    }
    
    // Выводим полученный текст для отладки
    printf("Полученный текст: '%s'\n", regular_text->text);
    
    // Проверяем, что текст содержит все нужные части
    if (strstr(regular_text->text, "This is") == NULL ||
        strstr(regular_text->text, "bold") == NULL ||
        strstr(regular_text->text, "text") == NULL) {
        printf("Ошибка: текст не содержит необходимых элементов\n");
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
    
    printf("Тест 3 пройден успешно\n");
    return 1;
}

// Тест 4: Проверка обработки различных HTML тегов
static int VTL_test_HtmlTagsProcessing() {
    printf("Тест 4: Проверка обработки различных HTML тегов\n");
    
    // Создаем HTML с разными тегами
    const char* html_str = "<p>Это <b>жирный</b>, <i>курсивный</i> и <s>зачеркнутый</s> текст</p>";
    VTL_publication_Text html_text;
    html_text.text = (VTL_publication_text_Symbol*)html_str;
    html_text.length = strlen(html_str);
    
    // Преобразуем в размеченный текст
    VTL_publication_MarkedText* marked_text = NULL;
    VTL_AppResult result = VTL_publication_text_InitFromHTML(&marked_text, &html_text);
    
    // Проверяем результат
    if (result != VTL_res_kOk) {
        printf("Ошибка преобразования HTML в размеченный текст: %d\n", result);
        return 0;
    }
    
    // Преобразуем обратно в HTML
    VTL_publication_Text* html_result = NULL;
    result = VTL_publication_marked_text_TransformToHTML(&html_result, marked_text);
    
    if (result != VTL_res_kOk) {
        printf("Ошибка преобразования размеченного текста в HTML: %d\n", result);
        VTL_publication_marked_text_Free(marked_text);
        return 0;
    }
    
    // Ожидаемый HTML может немного отличаться, но должен содержать основные теги
    int success = 1;
    if (strstr(html_result->text, "<b>") == NULL || 
        strstr(html_result->text, "</b>") == NULL ||
        strstr(html_result->text, "<i>") == NULL || 
        strstr(html_result->text, "</i>") == NULL ||
        strstr(html_result->text, "<s>") == NULL || 
        strstr(html_result->text, "</s>") == NULL) {
        printf("Ошибка: не все теги присутствуют в результате\n");
        success = 0;
    }
    
    // Освобождаем память
    VTL_publication_marked_text_Free(marked_text);
    VTL_publication_text_Free(html_result);
    
    if (success) {
        printf("Тест 4 пройден успешно\n");
        return 1;
    } else {
        return 0;
    }
}

// Главная функция с запуском тестов
int main() {
    int success = 1;
    
    // Запускаем все тесты
    success &= VTL_test_HtmlToMarkedTextSimple();
    success &= VTL_test_MarkedTextToHtml();
    success &= VTL_test_MarkedTextToRegularText();
    success &= VTL_test_HtmlTagsProcessing();
    
    if (success) {
        printf("Все тесты пройдены успешно!\n");
        return 0;
    } else {
        printf("Некоторые тесты не пройдены!\n");
        return 1;
    }
} 