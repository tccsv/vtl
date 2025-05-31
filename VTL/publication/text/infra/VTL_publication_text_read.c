#include <VTL/publication/text/infra/VTL_publication_text_read.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

VTL_AppResult VTL_pusblication_text_Read(VTL_publication_Text** pp_text, const VTL_Filename file_name)
{
    FILE* file = fopen(file_name, "rb");
    if (!file) {
        return VTL_res_kFileOpenErr;
    }
    
    // Получаем размер файла
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Выделяем память для текста
    *pp_text = (VTL_publication_Text*)malloc(sizeof(VTL_publication_Text));
    if (!*pp_text) {
        fclose(file);
        return VTL_res_kMemAllocErr;
    }
    
    // Выделяем память для содержимого
    (*pp_text)->text = (VTL_publication_text_Symbol*)malloc(file_size + 1);
    if (!(*pp_text)->text) {
        free(*pp_text);
        *pp_text = NULL;
        fclose(file);
        return VTL_res_kMemAllocErr;
    }
    
    // Читаем содержимое файла
    size_t read_size = fread((*pp_text)->text, 1, file_size, file);
    (*pp_text)->text[read_size] = '\0';  // Добавляем завершающий нуль
    (*pp_text)->length = read_size;
    
    fclose(file);
    return VTL_res_kOk;
}