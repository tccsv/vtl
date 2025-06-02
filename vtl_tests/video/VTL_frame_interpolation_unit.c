#include <vtl_tests/VTL_test_data.h>
#include <VTL/media_container/video/editor/VTL_frame_interpolation.h>
#include <VTL/media_container/video/VTL_video_data.h>
#include <string.h>

// Тестовые данные
static const char* TEST_INPUT_VIDEO = "test_input.mp4";
static const char* TEST_OUTPUT_VIDEO = "test_output.mp4";
static const VTL_InterpolationParams TEST_INTERPOLATION_PARAMS = {
    .width = 1920,
    .height = 1080,
    .fps = 60,
    .interpolation_factor = 2.0f
};

// Тестовые функции
static int VTL_test_frame_interpolation_with_opencl(void) {
    VTL_AppResult result = VTL_interpolate_video_frames(
        TEST_INPUT_VIDEO,
        TEST_OUTPUT_VIDEO,
        &TEST_INTERPOLATION_PARAMS
    );
    const char test_fail_message[] = "\nОшибка при интерполяции кадров с OpenCL. Ожидался успешный результат\n";
    return VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

static int VTL_test_frame_interpolation_with_invalid_input(void) {
    VTL_AppResult result = VTL_interpolate_video_frames(
        "nonexistent.mp4",
        TEST_OUTPUT_VIDEO,
        &TEST_INTERPOLATION_PARAMS
    );
    const char test_fail_message[] = "\nОшибка при интерполяции несуществующего видео. Ожидалась ошибка отсутствия файла\n";
    return VTL_test_CheckCondition(result == VTL_res_video_fs_r_kMissingFileErr, test_fail_message);
}

static int VTL_test_frame_interpolation_with_invalid_params(void) {
    VTL_InterpolationParams invalid_params = TEST_INTERPOLATION_PARAMS;
    invalid_params.width = 0; // Некорректная ширина
    VTL_AppResult result = VTL_interpolate_video_frames(
        TEST_INPUT_VIDEO,
        TEST_OUTPUT_VIDEO,
        &invalid_params
    );
    const char test_fail_message[] = "\nОшибка при интерполяции с некорректными параметрами. Ожидалась ошибка инициализации\n";
    return VTL_test_CheckCondition(result == VTL_res_ffmpeg_kInitError, test_fail_message);
}

static int VTL_test_frame_interpolation_with_invalid_output(void) {
    VTL_AppResult result = VTL_interpolate_video_frames(
        TEST_INPUT_VIDEO,
        "/invalid/path/output.mp4",
        &TEST_INTERPOLATION_PARAMS
    );
    const char test_fail_message[] = "\nОшибка при интерполяции с некорректным путем вывода. Ожидалась ошибка записи файла\n";
    return VTL_test_CheckCondition(result == VTL_res_video_fs_w_kFileIsBusyErr, test_fail_message);
}

int main(void) {
    int test_results = 1;
    
    // Запуск тестов
    test_results &= VTL_test_frame_interpolation_with_opencl();
    test_results &= VTL_test_frame_interpolation_with_invalid_input();
    test_results &= VTL_test_frame_interpolation_with_invalid_params();
    test_results &= VTL_test_frame_interpolation_with_invalid_output();
    
    return !test_results;
} 