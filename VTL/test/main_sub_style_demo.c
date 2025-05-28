#include <VTL/media_container/sub/VTL_sub_convert.h>
#include <VTL/media_container/sub/VTL_sub_style.h>
#include <VTL/media_container/sub/VTL_sub_burn.h>
#include <VTL/media_container/sub/infra/VTL_sub_read.h>
#include <VTL/media_container/sub/infra/VTL_sub_write.h>
#include <stdio.h>

int main(void)
{
    VTL_sub_StyleParams style_params;
    if (VTL_sub_StyleLoadFromJson("VTL/test/data/subtitle_style.json", &style_params) != 0) {
        fprintf(stderr, "Ошибка загрузки JSON стиля\n");
        return 1;
    }
    // 1. Конвертация SRT -> ASS с применением стиля
    // --- ОТЛАДКА: считаем сколько субтитров в исходном SRT ---
    VTL_sub_ReadSource* src = NULL;
    if (VTL_sub_ReadOpenSource("VTL/test/data/input.srt", &src) == 0) {
        VTL_sub_ReadMeta meta;
        if (VTL_sub_ReadMetaData(src, &meta) == 0) {
        }
        VTL_sub_ReadCloseSource(&src);
    }
    // ---
    printf("[DEBUG] Call VTL_sub_ConvertWithStyle: input=%s, output=%s, format_in=%d, format_out=%d\n", "VTL/test/data/input.srt", "VTL/test/data/output.ass", VTL_sub_format_kSRT, VTL_sub_format_kASS);
    fflush(stdout);
    VTL_AppResult result = VTL_sub_ConvertWithStyle(
        "VTL/test/data/input.srt", VTL_sub_format_kSRT,
        "VTL/test/data/output.ass", VTL_sub_format_kASS,
        &style_params);
    printf("[DEBUG] VTL_sub_ConvertWithStyle result: %d\n", result);
    fflush(stdout);
    // --- ОТЛАДКА: считаем сколько субтитров в output.ass ---
    if (VTL_sub_ReadOpenSource("VTL/test/data/output.ass", &src) == 0) {
        VTL_sub_ReadMeta meta;
        if (VTL_sub_ReadMetaData(src, &meta) == 0) {
        }
        VTL_sub_ReadCloseSource(&src);
    }
    // ---
    if (result == VTL_res_kOk) {
        printf("Subtitle conversion with style from JSON success!\n");
        // 2. Прожиг ASS в видео
        int burn_res = VTL_sub_BurnToVideo(
            "VTL/test/data/input.mp4",
            "VTL/test/data/output.ass",
            VTL_sub_format_kASS,
            "VTL/test/data/output_burned.mp4",
            &style_params
        );
        if (burn_res == 0) {
            printf("Burn-in subtitles to video success!\n");
            // 2. Конвертация ASS -> VTT с дефолтным стилем (style_params = NULL)
            result = VTL_sub_ConvertWithStyle(
                "VTL/test/data/output.ass", VTL_sub_format_kASS,
                "VTL/test/data/output.vtt", VTL_sub_format_kVTT,
                NULL
            );
            if (result == VTL_res_kOk) {
                printf("Subtitle conversion to VTT success!\n");
                return 0;
            }
        } else {
            printf("Burn-in subtitles to video failed: %d\n", burn_res);
        }
    } else {
        printf("Subtitle conversion failed: %d\n", result);
    }
    return result;
} 