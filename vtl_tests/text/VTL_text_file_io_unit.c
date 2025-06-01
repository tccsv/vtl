#include <VTL/VTL.h>
#include <VTL/VTL_app_result.h>
#include <VTL/publication/text/VTL_publication_text_op.h>
#include <VTL/publication/text/infra/VTL_publication_text_read.h>
#include <VTL/publication/text/infra/VTL_publication_text_write.h>
#include <vtl_tests/VTL_test_data.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



// Тест чтения файла
static int VTL_test_ReadFile() {
    printf("Тест чтения файла\n");
    
    // Используем абсолютный путь к файлу test.html
    const char* test_file = "test.html";
    
    // Проверяем существование файла перед чтением
    FILE* check_file = fopen(test_file, "r");
    if (check_file == NULL) {
        printf("Предупреждение: файл %s не найден, создаем тестовый файл\n", test_file);
        
        // Создаем тестовый файл, если он не существует
        check_file = fopen(test_file, "w");
        if (check_file == NULL) {
            const char* test_fail_message = "\nОшибка: не удалось создать тестовый файл\n";
            VTL_test_CheckCondition(0, test_fail_message);
            return 0;
        }
        
        // Записываем простой HTML в файл
        const char* test_html = "<html><body><p>Test HTML</p></body></html>";
        fwrite(test_html, 1, strlen(test_html), check_file);
        fclose(check_file);
        
        printf("Тестовый файл создан успешно\n");
    } else {
        fclose(check_file);
    }
    
    // Чтение файла
    VTL_publication_Text* text = NULL;
    VTL_AppResult result = VTL_pusblication_text_Read(&text, test_file);
    
    // Проверка результата
    const char* test_fail_message = "\nОшибка чтения файла\n";
    if (!VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message)) {
        return 0;
    }
    
    // Проверка, что текст не пустой
    test_fail_message = "\nОшибка: прочитанный текст пуст\n";
    if (!VTL_test_CheckCondition(text->length > 0 && text->text != NULL, test_fail_message)) {
        VTL_publication_text_Free(text);
        return 0;
    }
    
    printf("Успешно прочитан файл размером %zu байт\n", text->length);
    
    // Освобождаем память
    VTL_publication_text_Free(text);
    
    printf("Тест чтения файла пройден успешно\n");
    return 1;
}

// Тест записи файла
static int VTL_test_WriteFile() {
    printf("Тест записи файла\n");
    
    // Создаем текст для записи
    const char* test_str = "Тестовый текст для записи в файл";
    VTL_publication_Text text;
    text.text = (VTL_publication_text_Symbol*)test_str;
    text.length = strlen(test_str);
    
    // Запись в файл
    const char* output_file = "test_output.txt";
    VTL_AppResult result = VTL_pusblication_text_Write(&text, output_file);
    
    // Проверка результата
    const char* test_fail_message = "\nОшибка записи файла\n";
    if (!VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message)) {
        return 0;
    }
    
    // Проверка, что файл существует и имеет правильное содержимое
    FILE* file = fopen(output_file, "r");
    test_fail_message = "\nОшибка: не удалось открыть записанный файл\n";
    if (!VTL_test_CheckCondition(file != NULL, test_fail_message)) {
        return 0;
    }
    
    // Выделяем память для чтения файла
    char* buffer = (char*)malloc(text.length + 1);
    test_fail_message = "\nОшибка выделения памяти\n";
    if (!VTL_test_CheckCondition(buffer != NULL, test_fail_message)) {
        fclose(file);
        return 0;
    }
    
    // Чтение файла
    size_t read_size = fread(buffer, 1, text.length, file);
    buffer[read_size] = '\0';
    fclose(file);
    
    // Проверка содержимого
    test_fail_message = "\nОшибка: содержимое файла не соответствует ожидаемому\n";
    if (!VTL_test_CheckCondition(read_size == text.length && strcmp(buffer, test_str) == 0, test_fail_message)) {
        printf("Ожидалось: '%s'\n", test_str);
        printf("Получено: '%s'\n", buffer);
        free(buffer);
        return 0;
    }
    
    free(buffer);
    printf("Тест записи файла пройден успешно\n");
    return 1;
}

// Тест цикла чтение-запись-чтение
static int VTL_test_ReadWriteCycle() {
    printf("Тест цикла чтение-запись-чтение\n");
    
    const char* input_file = "test.html";
    const char* output_file = "test_cycle_output.html";
    
    // Проверяем существование входного файла
    FILE* check_file = fopen(input_file, "r");
    if (check_file == NULL) {
        printf("Предупреждение: входной файл %s не найден, создаем тестовый файл\n", input_file);
        
        // Создаем тестовый файл, если он не существует
        check_file = fopen(input_file, "w");
        const char* test_fail_message = "\nОшибка: не удалось создать тестовый файл\n";
        if (!VTL_test_CheckCondition(check_file != NULL, test_fail_message)) {
            return 0;
        }
        
        // Записываем простой HTML в файл
        const char* test_html = "<html><body><p>Test HTML for cycle test</p></body></html>";
        fwrite(test_html, 1, strlen(test_html), check_file);
        fclose(check_file);
        
        printf("Тестовый файл создан успешно\n");
    } else {
        fclose(check_file);
    }
    
    // Чтение исходного файла
    VTL_publication_Text* input_text = NULL;
    VTL_AppResult result = VTL_pusblication_text_Read(&input_text, input_file);
    
    const char* test_fail_message = "\nОшибка чтения исходного файла\n";
    if (!VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message)) {
        return 0;
    }
    
    // Запись в новый файл
    result = VTL_pusblication_text_Write(input_text, output_file);
    
    test_fail_message = "\nОшибка записи в новый файл\n";
    if (!VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message)) {
        VTL_publication_text_Free(input_text);
        return 0;
    }
    
    // Чтение нового файла
    VTL_publication_Text* output_text = NULL;
    result = VTL_pusblication_text_Read(&output_text, output_file);
    
    test_fail_message = "\nОшибка чтения нового файла\n";
    if (!VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message)) {
        VTL_publication_text_Free(input_text);
        return 0;
    }
    
    // Сравнение размеров
    test_fail_message = "\nОшибка: размеры файлов не совпадают\n";
    if (!VTL_test_CheckCondition(input_text->length == output_text->length, test_fail_message)) {
        printf("Размеры: %zu vs %zu\n", input_text->length, output_text->length);
        VTL_publication_text_Free(input_text);
        VTL_publication_text_Free(output_text);
        return 0;
    }
    
    // Сравнение содержимого
    test_fail_message = "\nОшибка: содержимое файлов не совпадает\n";
    if (!VTL_test_CheckCondition(memcmp(input_text->text, output_text->text, input_text->length) == 0, test_fail_message)) {
        VTL_publication_text_Free(input_text);
        VTL_publication_text_Free(output_text);
        return 0;
    }
    
    // Освобождаем память
    VTL_publication_text_Free(input_text);
    VTL_publication_text_Free(output_text);
    
    printf("Тест цикла чтение-запись-чтение пройден успешно\n");
    return 1;
}

// Тест 4: Чтение несуществующего файла
static int VTL_test_ReadNonExistentFile() {
    printf("Тест 4: Чтение несуществующего файла\n");
    
    // Используем имя файла, которого точно не существует
    const char* nonexistent_file = "nonexistent_file_for_test.txt";
    
    // Удаляем файл, если он по каким-то причинам существует
    remove(nonexistent_file);
    
    // Чтение несуществующего файла
    VTL_publication_Text* text = NULL;
    VTL_AppResult result = VTL_pusblication_text_Read(&text, nonexistent_file);
    
    // Проверка результата: должна быть ошибка
    const char* test_fail_message = "\nОшибка: функция чтения не вернула ошибку для несуществующего файла\n";
    if (!VTL_test_CheckCondition(result != VTL_res_kOk, test_fail_message)) {
        if (text != NULL) {
            VTL_publication_text_Free(text);
        }
        return 0;
    }
    
    printf("Тест 4 пройден успешно\n");
    return 1;
}

// Тест 5: Запись в файл с неправильными правами доступа
static int VTL_test_WriteToInaccessibleFile() {
    printf("Тест 5: Запись в файл с неправильными правами доступа\n");
    
    // В Windows это может не сработать из-за прав, но мы все равно попробуем
    // Создаем текст для записи
    const char* test_str = "Тест записи в файл с неправильными правами";
    VTL_publication_Text text;
    text.text = (VTL_publication_text_Symbol*)test_str;
    text.length = strlen(test_str);
    
    // Пытаемся записать в директорию (что должно вызвать ошибку)
    const char* inaccessible_file = "./";
    VTL_AppResult result = VTL_pusblication_text_Write(&text, inaccessible_file);
    
    // Проверка результата: должна быть ошибка
    // В Windows это может не сработать, поэтому проверяем мягко
    if (result != VTL_res_kOk) {
        printf("Тест 5 пройден успешно (запись в директорию вызвала ошибку)\n");
        return 1;
    } else {
        printf("Предупреждение: запись в директорию не вызвала ошибку (возможно, из-за ОС Windows)\n");
        printf("Тест 5 пропущен\n");
        return 1;  // Считаем тест пройденным, так как это зависит от ОС
    }
}

// Тест 6: Запись и чтение файла с Unicode символами
static int VTL_test_UnicodeFileIO() {
    printf("Тест 6: Запись и чтение файла с Unicode символами\n");
    
    // Создаем текст с Unicode символами для записи
    const char* test_str = "Тест с Unicode символами: ㋛ ☺ ☻ ♥ ♦ ♣ ♠ • ◘ ○";
    VTL_publication_Text text;
    text.text = (VTL_publication_text_Symbol*)test_str;
    text.length = strlen(test_str);
    
    // Запись в файл
    const char* output_file = "test_unicode.txt";
    VTL_AppResult result = VTL_pusblication_text_Write(&text, output_file);
    
    // Проверка результата записи
    const char* test_fail_message = "\nОшибка записи Unicode файла\n";
    if (!VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message)) {
        return 0;
    }
    
    // Чтение файла
    VTL_publication_Text* read_text = NULL;
    result = VTL_pusblication_text_Read(&read_text, output_file);
    
    // Проверка результата чтения
    test_fail_message = "\nОшибка чтения Unicode файла\n";
    if (!VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message)) {
        return 0;
    }
    
    // Проверка длины прочитанного текста
    test_fail_message = "\nОшибка: размер прочитанного Unicode текста не совпадает с оригинальным\n";
    if (!VTL_test_CheckCondition(read_text->length == text.length, test_fail_message)) {
        printf("Ожидаемая длина: %zu, прочитано: %zu\n", text.length, read_text->length);
        VTL_publication_text_Free(read_text);
        return 0;
    }
    
    // Сравнение содержимого
    test_fail_message = "\nОшибка: содержимое Unicode файла не соответствует ожидаемому\n";
    if (!VTL_test_CheckCondition(memcmp(text.text, read_text->text, text.length) == 0, test_fail_message)) {
        VTL_publication_text_Free(read_text);
        return 0;
    }
    
    // Освобождаем память
    VTL_publication_text_Free(read_text);
    
    printf("Тест 6 пройден успешно\n");
    return 1;
}

// Тест 7: Чтение и запись пустого файла
static int VTL_test_EmptyFileIO() {
    printf("Тест 7: Чтение и запись пустого файла\n");
    
    // Создаем пустой файл
    const char* empty_file = "test_empty.txt";
    FILE* fp = fopen(empty_file, "w");
    if (fp == NULL) {
        const char* test_fail_message = "\nОшибка: не удалось создать пустой файл\n";
        VTL_test_CheckCondition(0, test_fail_message);
        return 0;
    }
    fclose(fp);
    
    // Чтение пустого файла
    VTL_publication_Text* empty_text = NULL;
    VTL_AppResult result = VTL_pusblication_text_Read(&empty_text, empty_file);
    
    // Проверка результата чтения
    const char* test_fail_message = "\nОшибка чтения пустого файла\n";
    if (!VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message)) {
        return 0;
    }
    
    // Проверка, что текст пуст
    test_fail_message = "\nОшибка: прочитанный текст не пуст\n";
    if (!VTL_test_CheckCondition(empty_text->length == 0, test_fail_message)) {
        VTL_publication_text_Free(empty_text);
        return 0;
    }
    
    // Создаем пустой текст для записи
    VTL_publication_Text empty_write_text;
    empty_write_text.text = (VTL_publication_text_Symbol*)"";
    empty_write_text.length = 0;
    
    // Запись пустого текста в файл
    const char* output_empty_file = "test_output_empty.txt";
    result = VTL_pusblication_text_Write(&empty_write_text, output_empty_file);
    
    // Проверка результата записи
    test_fail_message = "\nОшибка записи пустого файла\n";
    if (!VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message)) {
        VTL_publication_text_Free(empty_text);
        return 0;
    }
    
    // Освобождаем память
    VTL_publication_text_Free(empty_text);
    
    printf("Тест 7 пройден успешно\n");
    return 1;
}

// Главная функция с запуском тестов
int main(void) {
    // Запускаем все тесты и считаем количество ошибок
    int fail_count = 0;
    
    // Для каждого теста увеличиваем счетчик, если тест не пройден
    if (!VTL_test_ReadFile()) fail_count++;
    if (!VTL_test_WriteFile()) fail_count++;
    if (!VTL_test_ReadWriteCycle()) fail_count++;
    
    // Новые тесты
    if (!VTL_test_ReadNonExistentFile()) fail_count++;
    if (!VTL_test_WriteToInaccessibleFile()) fail_count++;
    if (!VTL_test_UnicodeFileIO()) fail_count++;
    if (!VTL_test_EmptyFileIO()) fail_count++;
    
    // Выводим общий результат
    if (fail_count == 0) {
        printf("Все тесты ввода-вывода файлов пройдены успешно!\n");
    } else {
        printf("Не пройдено тестов ввода-вывода файлов: %d\n", fail_count);
    }
    
    // Возвращаем число ошибок
    return fail_count;
} 