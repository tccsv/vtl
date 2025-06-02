#include <vtl_tests/VTL_test_data.h>
#include <VTL/media_container/img/VTL_img_data.h>
#include <VTL/media_container/img/VTL_img_filters.h>
#include <VTL/VTL_app_result.h>

// Тест для VTL_img_data_init
int VTL_test_img_data_init(void)
{
    VTL_img_data_t img_data;
    VTL_AppResult result = VTL_img_data_init(&img_data);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_img_data_init\n";
    return !VTL_test_CheckCondition(result == VTL_IMG_SUCCESS, test_fail_message);
}

// Тест для VTL_img_data_free
int VTL_test_img_data_free(void)
{
    VTL_img_data_t img_data;
    VTL_img_data_init(&img_data);
    VTL_img_data_free(&img_data);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_img_data_free\n";
    return !VTL_test_CheckCondition(img_data.current_frame == NULL && 
                                  img_data.codec_ctx == NULL && 
                                  img_data.format_ctx == NULL && 
                                  img_data.filter_graph == NULL && 
                                  img_data.sws_ctx == NULL, test_fail_message);
}

// Тест для VTL_img_apply_filter
int VTL_test_img_apply_filter(void)
{
    VTL_img_data_t img_data;
    VTL_img_data_init(&img_data);
    
    // Создаем тестовый кадр
    img_data.current_frame = av_frame_alloc();
    img_data.current_frame->width = 640;
    img_data.current_frame->height = 480;
    img_data.current_frame->format = AV_PIX_FMT_YUV420P;
    
    VTL_AppResult result = VTL_img_apply_filter(&img_data, &VTL_img_filter_blur);
    VTL_img_data_free(&img_data);
    
    const char test_fail_message[] = "\nОшибка при тестировании VTL_img_apply_filter\n";
    return !VTL_test_CheckCondition(result == VTL_IMG_SUCCESS, test_fail_message);
}

// Тест для VTL_img_convert_format
int VTL_test_img_convert_format(void)
{
    VTL_img_data_t img_data;
    VTL_img_data_init(&img_data);
    
    // Создаем тестовый кадр
    img_data.current_frame = av_frame_alloc();
    img_data.current_frame->width = 640;
    img_data.current_frame->height = 480;
    img_data.current_frame->format = AV_PIX_FMT_YUV420P;
    
    VTL_AppResult result = VTL_img_convert_format(&img_data, AV_PIX_FMT_RGB24);
    VTL_img_data_free(&img_data);
    
    const char test_fail_message[] = "\nОшибка при тестировании VTL_img_convert_format\n";
    return !VTL_test_CheckCondition(result == VTL_IMG_SUCCESS, test_fail_message);
}

// Тест для VTL_img_GetAvailableFilters
int VTL_test_img_get_available_filters(void)
{
    const VTL_ImageFilter** filters = VTL_img_GetAvailableFilters();
    const char test_fail_message[] = "\nОшибка при тестировании VTL_img_GetAvailableFilters\n";
    return !VTL_test_CheckCondition(filters != NULL && filters[0] != NULL, test_fail_message);
}

int main(void)
{
    // Запускаем все тесты
    int result = 1;
    
    result &= VTL_test_img_data_init();
    result &= VTL_test_img_data_free();
    result &= VTL_test_img_apply_filter();
    result &= VTL_test_img_convert_format();
    result &= VTL_test_img_get_available_filters();
    
    return !result;
} 