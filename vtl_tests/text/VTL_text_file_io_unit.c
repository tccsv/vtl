#include <VTL/VTL.h>
#include <VTL/VTL_app_result.h>
#include <VTL/publication/text/VTL_publication_text_op.h>
#include <VTL/publication/text/infra/VTL_publication_text_read.h>
#include <VTL/publication/text/infra/VTL_publication_text_write.h>
#include <vtl_tests/VTL_test_data.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



// Тест 1: Чтение файла в VTL_publication_Text
static int VTL_test_ReadTextFromFile() {
    const char* filename = "test_read.txt";
    
    // Создаем тестовый файл
    FILE* test_file = fopen(filename, "wb");
    const char* test_content = "Hello World!";
    
    const char* test_fail_message = "\nОшибка создания тестового файла\n";
    if (!VTL_test_CheckCondition(test_file != NULL, test_fail_message)) {
        return 0;
    }
    
    fwrite(test_content, 1, strlen(test_content), test_file);
    fclose(test_file);
    
    // Читаем файл в VTL_publication_Text
    VTL_publication_Text* text = NULL;
    VTL_AppResult result = VTL_publication_text_Read(&text, filename);
    
    test_fail_message = "\nОшибка чтения файла в VTL_publication_Text\n";
    if (!VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message)) {
        remove(filename);
        return 0;
    }
    
    // Проверяем содержимое
    test_fail_message = "\nОшибка: неверное содержимое файла\n";
    if (!VTL_test_CheckCondition(
        text != NULL && 
        text->length == strlen(test_content) &&
        strncmp(text->text, test_content, text->length) == 0,
        test_fail_message)) {
        
        VTL_publication_text_Free(text);
        remove(filename);
        return 0;
    }
    
    // Очистка
    VTL_publication_text_Free(text);
    remove(filename);
    
    return 1;
}

// Тест 2: Запись VTL_publication_Text в файл
static int VTL_test_WriteTextToFile() {
    const char* filename = "test_write.txt";
    const char* test_content = "Test content for writing";
    
    // Создаем VTL_publication_Text
    VTL_publication_Text text;
    text.text = (VTL_publication_text_Symbol*)test_content;
    text.length = strlen(test_content);
    
    // Записываем в файл
    VTL_AppResult result = VTL_publication_text_Write(&text, filename);
    
    const char* test_fail_message = "\nОшибка записи VTL_publication_Text в файл\n";
    if (!VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message)) {
        remove(filename);
        return 0;
    }
    
    // Проверяем содержимое файла
    FILE* file = fopen(filename, "rb");
    test_fail_message = "\nОшибка открытия файла для проверки записи\n";
    if (!VTL_test_CheckCondition(file != NULL, test_fail_message)) {
        remove(filename);
        return 0;
    }
    
    // Получаем размер файла
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Читаем содержимое
    char* file_content = malloc(file_size + 1);
    fread(file_content, 1, file_size, file);
    file_content[file_size] = '\0';
    fclose(file);
    
    // Проверяем, что содержимое совпадает
    test_fail_message = "\nОшибка: записанное содержимое не совпадает\n";
    if (!VTL_test_CheckCondition(
        file_size == (long)strlen(test_content) &&
        strcmp(file_content, test_content) == 0,
        test_fail_message)) {
        
        free(file_content);
        remove(filename);
        return 0;
    }
    
    // Очистка
    free(file_content);
    remove(filename);
    
    return 1;
}

// Тест 3: Чтение размеченного текста из файла (упрощенный)
static int VTL_test_ReadMarkedTextFromFile() {
    const char* filename = "test_marked.txt";
    
    // Создаем тестовый файл с размеченным текстом
    FILE* test_file = fopen(filename, "wb");
    const char* test_content = "<b>Жирный</b> обычный <i>курсивный</i>";
    
    const char* test_fail_message = "\nОшибка создания тестового файла с размеченным текстом\n";
    if (!VTL_test_CheckCondition(test_file != NULL, test_fail_message)) {
        return 0;
    }
    
    fwrite(test_content, 1, strlen(test_content), test_file);
    fclose(test_file);
    
    // Читаем как обычный текст, а потом конвертируем в размеченный
    VTL_publication_Text* html_text = NULL;
    VTL_AppResult result = VTL_publication_text_Read(&html_text, filename);
    
    test_fail_message = "\nОшибка чтения HTML текста из файла\n";
    if (!VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message)) {
        remove(filename);
        return 0;
    }
    
    // Конвертируем в размеченный текст
    VTL_publication_MarkedText* marked_text = NULL;
    result = VTL_publication_text_InitFromHTML(&marked_text, html_text);
    
    test_fail_message = "\nОшибка преобразования HTML в размеченный текст\n";
    if (!VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message)) {
        VTL_publication_text_Free(html_text);
        remove(filename);
        return 0;
    }
    
    // Проверяем, что получили размеченный текст
    test_fail_message = "\nОшибка: размеченный текст не создан\n";
    if (!VTL_test_CheckCondition(marked_text != NULL && marked_text->parts != NULL, test_fail_message)) {
        VTL_publication_marked_text_Free(marked_text);
        VTL_publication_text_Free(html_text);
        remove(filename);
        return 0;
    }
    
    // Очистка
    VTL_publication_marked_text_Free(marked_text);
    VTL_publication_text_Free(html_text);
    remove(filename);
    
    return 1;
}

// Тест 4: Запись размеченного текста в файл (упрощенный)
static int VTL_test_WriteMarkedTextToFile() {
    const char* filename = "test_marked_write.txt";
    
    // Создаем размеченный текст
    VTL_publication_MarkedText marked_text;
    marked_text.length = 2;
    marked_text.parts = (VTL_publication_marked_text_Part*)malloc(2 * sizeof(VTL_publication_marked_text_Part));
    
    // Часть 1: жирный текст
    marked_text.parts[0].text = strdup("Bold");
    marked_text.parts[0].length = 4;
    marked_text.parts[0].type = VTL_TEXT_MODIFICATION_BOLD;
    
    // Часть 2: обычный текст
    marked_text.parts[1].text = strdup(" text");
    marked_text.parts[1].length = 5;
    marked_text.parts[1].type = 0;
    
    // Конвертируем в HTML
    VTL_publication_Text* html_text = NULL;
    VTL_AppResult result = VTL_publication_marked_text_TransformToHTML(&html_text, &marked_text);
    
    const char* test_fail_message = "\nОшибка преобразования размеченного текста в HTML\n";
    if (!VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message)) {
        // Освобождаем память
        for (size_t i = 0; i < marked_text.length; i++) {
            free(marked_text.parts[i].text);
        }
        free(marked_text.parts);
        remove(filename);
        return 0;
    }
    
    // Записываем HTML в файл
    result = VTL_publication_text_Write(html_text, filename);
    
    test_fail_message = "\nОшибка записи HTML в файл\n";
    if (!VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message)) {
        // Освобождаем память
        VTL_publication_text_Free(html_text);
        for (size_t i = 0; i < marked_text.length; i++) {
            free(marked_text.parts[i].text);
        }
        free(marked_text.parts);
        remove(filename);
        return 0;
    }
    
    // Проверяем, что файл создан
    FILE* file = fopen(filename, "rb");
    test_fail_message = "\nОшибка: файл не создан\n";
    if (!VTL_test_CheckCondition(file != NULL, test_fail_message)) {
        // Освобождаем память
        VTL_publication_text_Free(html_text);
        for (size_t i = 0; i < marked_text.length; i++) {
            free(marked_text.parts[i].text);
        }
        free(marked_text.parts);
        remove(filename);
        return 0;
    }
    
    fclose(file);
    
    // Освобождаем память
    VTL_publication_text_Free(html_text);
    for (size_t i = 0; i < marked_text.length; i++) {
        free(marked_text.parts[i].text);
    }
    free(marked_text.parts);
    remove(filename);
    
    return 1;
}

// Тест 5: Чтение несуществующего файла
static int VTL_test_ReadNonExistentFile() {
    const char* filename = "non_existent_file.txt";
    
    // Пытаемся прочитать несуществующий файл
    VTL_publication_Text* text = NULL;
    VTL_AppResult result = VTL_publication_text_Read(&text, filename);
    
    // Ожидаем ошибку
    const char* test_fail_message = "\nОшибка: несуществующий файл должен приводить к ошибке\n";
    if (!VTL_test_CheckCondition(result != VTL_res_kOk, test_fail_message)) {
        if (text) VTL_publication_text_Free(text);
        return 0;
    }
    
    return 1;
}

// Тест 6: Запись в недоступный файл 
static int VTL_test_WriteToInaccessibleFile() {
    // Создаем VTL_publication_Text
    const char* test_content = "Test content";
    VTL_publication_Text text;
    text.text = (VTL_publication_text_Symbol*)test_content;
    text.length = strlen(test_content);
    
    // Пытаемся записать в недоступный путь (корневой каталог системы)
    const char* inaccessible_filename = "/root/inaccessible_file.txt";
    VTL_AppResult result = VTL_publication_text_Write(&text, inaccessible_filename);
    
    // Ожидаем ошибку (на большинстве систем запись в /root недоступна)
    const char* test_fail_message = "\nОшибка: запись в недоступный файл должна приводить к ошибке\n";
    if (!VTL_test_CheckCondition(result != VTL_res_kOk, test_fail_message)) {
        return 0;
    }
    
    return 1;
}

// Тест 7: Работа с файлами в Unicode
static int VTL_test_UnicodeFileIO() {
    const char* filename = "test_unicode.txt";
    const char* unicode_content = "Тест с русскими символами: привет мир!";
    
    // Создаем VTL_publication_Text с Unicode содержимым
    VTL_publication_Text text;
    text.text = (VTL_publication_text_Symbol*)unicode_content;
    text.length = strlen(unicode_content);
    
    // Записываем в файл
    VTL_AppResult result = VTL_publication_text_Write(&text, filename);
    
    const char* test_fail_message = "\nОшибка записи Unicode в файл\n";
    if (!VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message)) {
        remove(filename);
        return 0;
    }
    
    // Читаем обратно
    VTL_publication_Text* read_text = NULL;
    result = VTL_publication_text_Read(&read_text, filename);
    
    test_fail_message = "\nОшибка чтения Unicode из файла\n";
    if (!VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message)) {
        remove(filename);
        return 0;
    }
    
    // Проверяем, что содержимое совпадает
    test_fail_message = "\nОшибка: прочитанное Unicode содержимое не совпадает\n";
    if (!VTL_test_CheckCondition(
        read_text != NULL && 
        read_text->length == text.length &&
        strncmp(read_text->text, text.text, text.length) == 0,
        test_fail_message)) {
        
        VTL_publication_text_Free(read_text);
        remove(filename);
        return 0;
    }
    
    // Очистка
    VTL_publication_text_Free(read_text);
    remove(filename);
    
    return 1;
}

// Тест 8: Работа с пустыми файлами
static int VTL_test_EmptyFileIO() {
    const char* filename = "test_empty.txt";
    
    // Создаем пустой файл
    FILE* empty_file = fopen(filename, "wb");
    const char* test_fail_message = "\nОшибка создания пустого файла\n";
    if (!VTL_test_CheckCondition(empty_file != NULL, test_fail_message)) {
        return 0;
    }
    fclose(empty_file);
    
    // Читаем пустой файл
    VTL_publication_Text* text = NULL;
    VTL_AppResult result = VTL_publication_text_Read(&text, filename);
    
    test_fail_message = "\nОшибка чтения пустого файла\n";
    if (!VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message)) {
        remove(filename);
        return 0;
    }
    
    // Проверяем, что получили пустой текст
    test_fail_message = "\nОшибка: пустой файл должен давать текст длины 0\n";
    if (!VTL_test_CheckCondition(text != NULL && text->length == 0, test_fail_message)) {
        VTL_publication_text_Free(text);
        remove(filename);
        return 0;
    }
    
    // Очистка
    VTL_publication_text_Free(text);
    remove(filename);
    
    return 1;
}

// Главная функция с запуском тестов
int main(void)
{
    // Запускаем все тесты и считаем количество ошибок
    int fail_count = 0;
    
    // Для каждого теста увеличиваем счетчик, если тест не пройден
    if (!VTL_test_ReadTextFromFile()) fail_count++;
    if (!VTL_test_WriteTextToFile()) fail_count++;
    if (!VTL_test_ReadMarkedTextFromFile()) fail_count++;
    if (!VTL_test_WriteMarkedTextToFile()) fail_count++;
    
    // Новые тесты
    if (!VTL_test_ReadNonExistentFile()) fail_count++;
    if (!VTL_test_WriteToInaccessibleFile()) fail_count++;
    if (!VTL_test_UnicodeFileIO()) fail_count++;
    if (!VTL_test_EmptyFileIO()) fail_count++;
    
    // Возвращаем число ошибок
    return fail_count;
} 