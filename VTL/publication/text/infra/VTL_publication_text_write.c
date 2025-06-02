#include <VTL/publication/text/infra/VTL_publication_text_write.h>
#include <stdio.h>

VTL_AppResult VTL_publication_text_Write(VTL_publication_Text* p_text, const VTL_Filename file_name)
{
    if (!p_text || !p_text->text || !file_name) {
        return VTL_res_kInvalidParamErr;
    }
    
    FILE* file = fopen(file_name, "wb");
    if (!file) {
        return VTL_res_kFileOpenErr;
    }
    
    // Записываем текст в файл
    size_t written = fwrite(p_text->text, 1, p_text->length, file);
    fclose(file);
    
    if (written != p_text->length) {
        return VTL_res_kFileWriteErr;
    }
    
    return VTL_res_kOk;
}