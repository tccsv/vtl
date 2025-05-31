#include <VTL/VTL.h>
#include <VTL/VTL_app_result.h>
#include <VTL/publication/text/VTL_publication_text_op.h>
#include <VTL/publication/text/infra/VTL_publication_text_read.h>
#include <VTL/publication/text/infra/VTL_publication_text_write.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>



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
            printf("Ошибка: не удалось создать тестовый файл\n");
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
    if (result != VTL_res_kOk) {
        printf("Ошибка чтения файла: %d\n", result);
        return 0;
    }
    
    // Проверка, что текст не пустой
    if (text->length == 0 || text->text == NULL) {
        printf("Ошибка: прочитанный текст пуст\n");
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
    if (result != VTL_res_kOk) {
        printf("Ошибка записи файла: %d\n", result);
        return 0;
    }
    
    // Проверка, что файл существует и имеет правильное содержимое
    FILE* file = fopen(output_file, "r");
    if (file == NULL) {
        printf("Ошибка: не удалось открыть записанный файл\n");
        return 0;
    }
    
    // Выделяем память для чтения файла
    char* buffer = (char*)malloc(text.length + 1);
    if (buffer == NULL) {
        printf("Ошибка выделения памяти\n");
        fclose(file);
        return 0;
    }
    
    // Чтение файла
    size_t read_size = fread(buffer, 1, text.length, file);
    buffer[read_size] = '\0';
    fclose(file);
    
    // Проверка содержимого
    if (read_size != text.length || strcmp(buffer, test_str) != 0) {
        printf("Ошибка: содержимое файла не соответствует ожидаемому\n");
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
        if (check_file == NULL) {
            printf("Ошибка: не удалось создать тестовый файл\n");
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
    
    if (result != VTL_res_kOk) {
        printf("Ошибка чтения исходного файла: %d\n", result);
        return 0;
    }
    
    // Запись в новый файл
    result = VTL_pusblication_text_Write(input_text, output_file);
    
    if (result != VTL_res_kOk) {
        printf("Ошибка записи в новый файл: %d\n", result);
        VTL_publication_text_Free(input_text);
        return 0;
    }
    
    // Чтение нового файла
    VTL_publication_Text* output_text = NULL;
    result = VTL_pusblication_text_Read(&output_text, output_file);
    
    if (result != VTL_res_kOk) {
        printf("Ошибка чтения нового файла: %d\n", result);
        VTL_publication_text_Free(input_text);
        return 0;
    }
    
    // Сравнение размеров
    if (input_text->length != output_text->length) {
        printf("Ошибка: размеры файлов не совпадают (%zu vs %zu)\n", 
               input_text->length, output_text->length);
        VTL_publication_text_Free(input_text);
        VTL_publication_text_Free(output_text);
        return 0;
    }
    
    // Сравнение содержимого
    if (memcmp(input_text->text, output_text->text, input_text->length) != 0) {
        printf("Ошибка: содержимое файлов не совпадает\n");
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

// Главная функция с запуском тестов
int main() {
    int success = 1;
    
    // Запускаем все тесты
    success &= VTL_test_ReadFile();
    success &= VTL_test_WriteFile();
    success &= VTL_test_ReadWriteCycle();
    
    if (success) {
        printf("Все тесты ввода-вывода файлов пройдены успешно!\n");
        return 0;
    } else {
        printf("Некоторые тесты ввода-вывода файлов не пройдены!\n");
        return 1;
    }
} 