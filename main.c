#include "VTL/media_container/img/VTL_img.h"
#include "VTL/media_container/img/VTL_img_data.h"
#include "VTL/media_container/img/VTL_img_filters.h"
#include <stdio.h>
#include <libavutil/log.h>

VTL_AppResult test_image_processing(void) {
    printf("Starting image processing...\n");
    VTL_AppResult result = VTL_res_kOk;
    
    printf("Initializing image context...\n");
    VTL_ImageContext* ctx = VTL_img_context_Init();
    if (!ctx) {
        printf("Failed to initialize context\n");
        VTL_console_out_PotencialErr(VTL_res_video_fs_r_kMissingFileErr);
        return VTL_res_video_fs_r_kMissingFileErr;
    }
    printf("Context initialized successfully\n");

    const char* input_path = "test_images/Lenna.png";
    printf("Checking if file exists: %s\n", input_path);
    if (!VTL_img_CheckFileExists(input_path)) {
        printf("File not found\n");
        VTL_console_out_PotencialErr(VTL_res_video_fs_r_kMissingFileErr);
        VTL_img_context_Cleanup(ctx);
        return VTL_res_video_fs_r_kMissingFileErr;
    }
    printf("File exists\n");

    printf("Loading image...\n");
    result = VTL_img_LoadImage(input_path, ctx);
    if (result != VTL_res_kOk) {
        printf("Failed to load image\n");
        VTL_console_out_PotencialErr(result);
        VTL_img_context_Cleanup(ctx);
        return result;
    }
    printf("Image loaded successfully\n");

    printf("Setting up filters...\n");
    VTL_ImageFilter filters[] = {
        {"blur", "Размытие изображения", "boxblur=5:1", NULL},
        {"sepia", "Сепия эффект", "colorchannelmixer=.393:.769:.189:0:.349:.686:.168:0:.272:.534:.131", NULL}
    };
    const int num_filters = sizeof(filters) / sizeof(filters[0]);

    for (int i = 0; i < num_filters; i++) {
        printf("Applying filter %d: %s\n", i + 1, filters[i].name);
        result = VTL_img_ApplyFilter(ctx, &filters[i]);
        if (result != VTL_res_kOk) {
            printf("Failed to apply filter %d\n", i + 1);
            VTL_console_out_PotencialErr(result);
            VTL_img_context_Cleanup(ctx);
            return result;
        }
        printf("Filter %d applied successfully\n", i + 1);
    }

    // Сохранение результата
    const char* output_path = "test_images/Lenna_processed.png";
    printf("Saving result to: %s\n", output_path);
    result = VTL_img_SaveImage(output_path, ctx);
    if (result != VTL_res_kOk) {
        printf("Failed to save image\n");
        VTL_console_out_PotencialErr(result);
    } else {
        printf("Image saved successfully\n");
    }

    // Очистка
    printf("Cleaning up...\n");
    VTL_img_context_Cleanup(ctx);
    printf("Cleanup completed\n");
    return result;
}

int main(void) {
    printf("Starting image processing test...\n");
    
    // Инициализация контекста
    VTL_ImageContext* ctx = VTL_img_context_Init();
    if (!ctx) {
        printf("Failed to initialize context\n");
        return -1;
    }
    
    // Загрузка изображения
    const char* input_path = "test_images/Lenna.png";
    VTL_AppResult result = VTL_img_LoadImage(input_path, ctx);
    if (result != VTL_res_kOk) {
        printf("Failed to load image: %d\n", result);
        VTL_img_context_Cleanup(ctx);
        return result;
    }
    
    // Применение фильтра размытия
    const VTL_ImageFilter* blur_filter = &VTL_img_filter_blur;
    result = VTL_img_ApplyFilter(ctx, blur_filter);
    if (result != VTL_res_kOk) {
        printf("Failed to apply blur filter: %d\n", result);
        VTL_img_context_Cleanup(ctx);
        return result;
    }
    
    // Сохранение результата
    const char* output_path = "test_images/Lenna_blurred.png";
    result = VTL_img_SaveImage(output_path, ctx);
    if (result != VTL_res_kOk) {
        printf("Failed to save image: %d\n", result);
        VTL_img_context_Cleanup(ctx);
        return result;
    }
    
    // Очистка
    VTL_img_context_Cleanup(ctx);
    printf("Test completed successfully\n");
    return 0;
}