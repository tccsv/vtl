#include <vtl_tests/VTL_test_data.h>
#include <VTL/publication/VTL_publication.h>
#include <VTL/publication/VTL_publication_data.h>
#include <VTL/VTL_content_platform_flags.h>
#include <VTL/VTL_publication_markup_text_flags.h>
#include <VTL/VTL_app_result.h>

// Тест для VTL_SheduleMarkedText
int test_SheduleMarkedText(void)
{
    const VTL_Filename test_file = "test.txt";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    const VTL_publication_marked_text_MarkupType markup_type = VTL_markup_type_kTelegramMD;
    
    VTL_AppResult result = VTL_SheduleMarkedText(test_file, flags, markup_type, 0);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_SheduleMarkedText\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

// Тест для VTL_PubicateMarkedText
int test_PubicateMarkedText(void)
{
    const VTL_Filename test_file = "test.txt";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    const VTL_publication_marked_text_MarkupType markup_type = VTL_markup_type_kTelegramMD;
    
    VTL_AppResult result = VTL_PubicateMarkedText(test_file, flags, markup_type);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_PubicateMarkedText\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

// Тест для VTL_SheduleText
int test_SheduleText(void)
{
    const VTL_Filename test_file = "test.txt";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    
    VTL_AppResult result = VTL_SheduleText(test_file, flags);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_SheduleText\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

// Тест для VTL_PubicateText
int test_PubicateText(void)
{
    const VTL_Filename test_file = "test.txt";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    
    VTL_AppResult result = VTL_PubicateText(test_file, flags);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_PubicateText\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

// Тест для VTL_ShedulePhoto
int test_ShedulePhoto(void)
{
    const VTL_Filename test_file = "test.jpg";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    
    VTL_AppResult result = VTL_ShedulePhoto(test_file, flags);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_ShedulePhoto\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

// Тест для VTL_PubicatePhoto
int test_PubicatePhoto(void)
{
    const VTL_Filename test_file = "test.jpg";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    
    VTL_AppResult result = VTL_PubicatePhoto(test_file, flags);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_PubicatePhoto\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

// Тест для VTL_ShedulePhotoWithMarkedText
int test_ShedulePhotoWithMarkedText(void)
{
    const VTL_Filename photo_file = "test.jpg";
    const VTL_Filename text_file = "test.txt";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    const VTL_publication_marked_text_MarkupType markup_type = VTL_markup_type_kTelegramMD;
    
    VTL_AppResult result = VTL_ShedulePhotoWithMarkedText(photo_file, text_file, flags, markup_type);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_ShedulePhotoWithMarkedText\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

// Тест для VTL_PubicatePhotoWithMarkedText
int test_PubicatePhotoWithMarkedText(void)
{
    const VTL_Filename photo_file = "test.jpg";
    const VTL_Filename text_file = "test.txt";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    const VTL_publication_marked_text_MarkupType markup_type = VTL_markup_type_kTelegramMD;
    
    VTL_AppResult result = VTL_PubicatePhotoWithMarkedText(photo_file, text_file, flags, markup_type);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_PubicatePhotoWithMarkedText\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

// Тест для VTL_ShedulePhotoWithText
int test_ShedulePhotoWithText(void)
{
    const VTL_Filename photo_file = "test.jpg";
    const VTL_Filename text_file = "test.txt";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    
    VTL_AppResult result = VTL_ShedulePhotoWithText(photo_file, text_file, flags);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_ShedulePhotoWithText\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

// Тест для VTL_PubicatePhotoWithText
int test_PubicatePhotoWithText(void)
{
    const VTL_Filename photo_file = "test.jpg";
    const VTL_Filename text_file = "test.txt";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    
    VTL_AppResult result = VTL_PubicatePhotoWithText(photo_file, text_file, flags);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_PubicatePhotoWithText\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

// Тест для VTL_SheduleVideo
int test_SheduleVideo(void)
{
    const VTL_Filename test_file = "test.mp4";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    
    VTL_AppResult result = VTL_SheduleVideo(test_file, flags);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_SheduleVideo\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

// Тест для VTL_PubicateVideo
int test_PubicateVideo(void)
{
    const VTL_Filename test_file = "test.mp4";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    
    VTL_AppResult result = VTL_PubicateVideo(test_file, flags);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_PubicateVideo\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

// Тест для VTL_SheduleVideoWithText
int test_SheduleVideoWithText(void)
{
    const VTL_Filename video_file = "test.mp4";
    const VTL_Filename text_file = "test.txt";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    
    VTL_AppResult result = VTL_SheduleVideoWithText(video_file, text_file, flags);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_SheduleVideoWithText\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

// Тест для VTL_PubicateVideoWithText
int test_PubicateVideoWithText(void)
{
    const VTL_Filename video_file = "test.mp4";
    const VTL_Filename text_file = "test.txt";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    
    VTL_AppResult result = VTL_PubicateVideoWithText(video_file, text_file, flags);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_PubicateVideoWithText\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

// Тест для VTL_SheduleVideoWithMarkedText
int test_SheduleVideoWithMarkedText(void)
{
    const VTL_Filename video_file = "test.mp4";
    const VTL_Filename text_file = "test.txt";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    const VTL_publication_marked_text_MarkupType markup_type = VTL_markup_type_kTelegramMD;
    
    VTL_AppResult result = VTL_SheduleVideoWithMarkedText(video_file, text_file, flags, markup_type);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_SheduleVideoWithMarkedText\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

// Тест для VTL_PubicateVideoWithMarkedText
int test_PubicateVideoWithMarkedText(void)
{
    const VTL_Filename video_file = "test.mp4";
    const VTL_Filename text_file = "test.txt";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    const VTL_publication_marked_text_MarkupType markup_type = VTL_markup_type_kTelegramMD;
    
    VTL_AppResult result = VTL_PubicateVideoWithMarkedText(video_file, text_file, flags, markup_type);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_PubicateVideoWithMarkedText\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

// Тест для VTL_SheduleVideoWithInnerSub
int test_SheduleVideoWithInnerSub(void)
{
    const VTL_Filename video_file = "test.mp4";
    const VTL_Filename sub_file = "test.srt";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    
    VTL_AppResult result = VTL_SheduleVideoWithInnerSub(video_file, sub_file, flags);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_SheduleVideoWithInnerSub\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

// Тест для VTL_PubicateVideoWithInnerSub
int test_PubicateVideoWithInnerSub(void)
{
    const VTL_Filename video_file = "test.mp4";
    const VTL_Filename sub_file = "test.srt";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    
    VTL_AppResult result = VTL_PubicateVideoWithInnerSub(video_file, sub_file, flags);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_PubicateVideoWithInnerSub\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

int main(void)
{
    // Запускаем все тесты
    int result = 1;
    
    result &= test_SheduleMarkedText();
    result &= test_PubicateMarkedText();
    result &= test_SheduleText();
    result &= test_PubicateText();
    result &= test_ShedulePhoto();
    result &= test_PubicatePhoto();
    result &= test_ShedulePhotoWithMarkedText();
    result &= test_PubicatePhotoWithMarkedText();
    result &= test_ShedulePhotoWithText();
    result &= test_PubicatePhotoWithText();
    result &= test_SheduleVideo();
    result &= test_PubicateVideo();
    result &= test_SheduleVideoWithText();
    result &= test_PubicateVideoWithText();
    result &= test_SheduleVideoWithMarkedText();
    result &= test_PubicateVideoWithMarkedText();
    result &= test_SheduleVideoWithInnerSub();
    result &= test_PubicateVideoWithInnerSub();
    
    return !result;
} 