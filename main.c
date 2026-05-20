#include <VTL/media_container/img/VTL_media_container_img_core.h>
#include <VTL/VTL_app_result.h>
#include <stdio.h>

int main()
{
    printf("VTL Image Processing Demo\n");

    // Инициализация контекста
    VTL_ImageContext* ctx = VTL_img_context_Init();
    if (!ctx) {
        printf("Failed to initialize VTL Image Context\n");
        return 1;
    }

    // Загрузка изображения
    const char* input_path = "./test_images/2-1.jpg";
    printf("Loading image: %s\n", input_path);
    VTL_AppResult res = VTL_img_LoadImage(input_path, ctx);
    if (res != VTL_res_kOk) {
        printf("Failed to load image '%s', error code: %d\n", input_path, (int)res);
        VTL_img_context_Cleanup(ctx);
        return 1;
    }

    // Настройка фильтра (черно-белый)
    VTL_ImageFilter black_white_filter = {
            .name = "Grayscale",
            .description = "Converts the image to black and white",
            .filter_desc = "format=gray",
            .apply = NULL
    };

    // Применение фильтра
    printf("Applying grayscale filter...\n");
    res = VTL_img_ApplyFilter(ctx, &black_white_filter);
    if (res != VTL_res_kOk) {
        printf("Failed to apply filter, error code: %d\n", (int)res);
        VTL_img_context_Cleanup(ctx);
        return 1;
    }

    // Сохранение результата
    const char* output_path = "./test_images/output.jpg";
    printf("Saving processed image to: %s\n", output_path);
    res = VTL_img_SaveImage(output_path, ctx);
    if (res != VTL_res_kOk) {
        printf("Failed to save image to '%s', error code: %d\n", output_path, (int)res);
        VTL_img_context_Cleanup(ctx);
        return 1;
    }

    printf("Successfully processed image and saved to %s\n", output_path);

    // Очистка ресурсов
    VTL_img_context_Cleanup(ctx);

    return 0;
}