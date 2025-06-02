
#include <stdio.h>
#include <stdlib.h>
#include <VTL/media_container/audio/infra/VTL_audio_read.h>
#include <VTL/media_container/audio/infra/VTL_audio_write.h>
#include <VTL/media_container/audio/VTL_audio_data.h>
#include "VTL/media_container/img/VTL_img.h"
#include "VTL/media_container/img/VTL_img_data.h"
#include "VTL/media_container/img/VTL_img_filters.h"
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
    //audio
    VTL_audio_File* input = NULL;
    if (VTL_audio_read_OpenSource(&input, "test_audios/input.mp3") != VTL_res_kOk) {
        printf("Ошибка открытия input.mp3\n");
        return 1;
    }

    VTL_audio_MetaData input_meta = {0};
    int input_has_meta = 0;
    if (fread(&input_meta, sizeof(VTL_audio_MetaData), 1, input) == 1) {
        input_has_meta = 1;
        printf("В input.mp3 найдены метаданные: bitrate=%u, sample_rate=%u, channels=%u, volume=%u\n",
               input_meta.params.bitrate, input_meta.params.sample_rate, input_meta.params.num_channels, input_meta.params.volume);
    } else {
        printf("В input.mp3 метаданные не найдены или файл меньше размера структуры.\n");
        fseek(input, 0, SEEK_SET); 
    }

    fseek(input, 0, SEEK_END);
    size_t filesize = ftell(input);
    size_t data_offset = input_has_meta ? sizeof(VTL_audio_MetaData) : 0;
    size_t data_size = filesize - data_offset;
    fseek(input, data_offset, SEEK_SET);

    VTL_audio_MetaData meta;
    meta.data_size = data_size;
    if (input_has_meta) {
        meta.params = input_meta.params;
    } else {
        memset(&meta.params, 0, sizeof(meta.params));
    }
    VTL_audio_params_BitrateSet(&meta.params, 128000);
    VTL_audio_params_CodecSet(&meta.params, VTL_audio_codec_kDefault);
    VTL_audio_params_SampleRateSet(&meta.params, 44100);
    VTL_audio_params_NumChannelsSet(&meta.params, 2);
    VTL_audio_params_VolumeSet(&meta.params, 100);

    VTL_audio_Data* data = (VTL_audio_Data*)malloc(sizeof(VTL_audio_Data));
    data->data_size = data_size;
    data->data = (char*)malloc(data_size);
    if (fread(data->data, 1, data_size, input) != data_size) {
        printf("Ошибка чтения данных\n");
        fclose(input);
        free(data->data);
        free(data);
        return 1;
    }
    fclose(input);

    VTL_audio_File* output = NULL;
    if (VTL_audio_OpenOutput(&output, "test_audios/output.mp3") != VTL_res_kOk) {
        printf("Ошибка открытия output.mp3\n");
        free(data->data);
        free(data);
        return 1;
    }

    fwrite(&meta, sizeof(VTL_audio_MetaData), 1, output); 

    if (VTL_audio_WritePart(data, output) != VTL_res_kOk) {
        printf("Ошибка записи данных\n");
    }

    VTL_audio_CloseOutput(output);

    VTL_audio_File* check_file = NULL;
    if (VTL_audio_read_OpenSource(&check_file, "test_audios/output.mp3") != VTL_res_kOk) {
        printf("Ошибка повторного открытия output.mp3\n");
        free(data->data);
        free(data);
        return 1;
    }

    VTL_audio_MetaData out_meta;
    if (fread(&out_meta, sizeof(VTL_audio_MetaData), 1, check_file) != 1) {
        printf("Ошибка чтения метаданных из output.mp3\n");
        fclose(check_file);
        free(data->data);
        free(data);
        return 1;
    }

    fseek(check_file, 0, SEEK_END);
    size_t out_filesize = ftell(check_file);
    size_t out_data_size = out_filesize - sizeof(VTL_audio_MetaData);
    fseek(check_file, sizeof(VTL_audio_MetaData), SEEK_SET);
    
    VTL_audio_Data* out_data = (VTL_audio_Data*)malloc(sizeof(VTL_audio_Data));
    out_data->data_size = out_data_size;
    out_data->data = (char*)malloc(out_data_size);
    if (fread(out_data->data, 1, out_data_size, check_file) != out_data_size) {
        printf("Ошибка чтения данных из output.mp3\n");
        fclose(check_file);
        free(out_data->data);
        free(out_data);
        free(data->data);
        free(data);
        return 1;
    }
    fclose(check_file);

    if (out_data->data_size != data->data_size) {
        printf("Размер данных не совпадает: %zu vs %zu\n", out_data->data_size, data->data_size);
    } else {
        printf("Размер данных совпадает: %zu байт\n", out_data->data_size);
    }

    int diff = memcmp(data->data, out_data->data, data->data_size);
    if (diff == 0) {
        printf("Данные совпадают.\n");
    } else {
        printf("Данные отличаются!\n");
    }

    printf("Параметры output: bitrate=%u, sample_rate=%u, channels=%u, volume=%u\n",
           out_meta.params.bitrate, out_meta.params.sample_rate, out_meta.params.num_channels, out_meta.params.volume);

    free(out_data->data);
    free(out_data);
    free(data->data);
    free(data);
    //audio end
    
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