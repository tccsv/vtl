#include <stdio.h>
#include <stdlib.h>
#include <VTL/media_container/audio/infra/VTL_audio_read.h>
#include <VTL/media_container/audio/infra/VTL_audio_write.h>
#include <VTL/media_container/audio/VTL_audio_data.h>

int main(void)
{
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

    printf("Готово!\n");
    return 0;
}