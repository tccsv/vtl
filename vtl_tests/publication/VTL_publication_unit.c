#include <vtl_tests/VTL_test_data.h>
#include <VTL/publication/VTL_publication.h>
#include <VTL/publication/VTL_publication_data.h>
#include <VTL/VTL_content_platform_flags.h>
#include <VTL/VTL_publication_markup_text_flags.h>
#include <VTL/VTL_app_result.h>
#include <stdbool.h>

// Тест для VTL_SheduleMarkedText
bool VTL_test_SheduleMarkedText(void)
{
    const VTL_Filename test_file = "test.txt";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    const VTL_publication_marked_text_MarkupType markup_type = VTL_markup_type_kTelegramMD;
    
    VTL_AppResult result = VTL_SheduleMarkedText(test_file, flags, markup_type, 0);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_SheduleMarkedText\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

// Тест для VTL_PubicateMarkedText
bool VTL_test_PubicateMarkedText(void)
{
    const VTL_Filename test_file = "test.txt";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    const VTL_publication_marked_text_MarkupType markup_type = VTL_markup_type_kTelegramMD;
    
    VTL_AppResult result = VTL_PubicateMarkedText(test_file, flags, markup_type);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_PubicateMarkedText\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

// Тест для VTL_SheduleText
bool VTL_test_SheduleText(void)
{
    const VTL_Filename test_file = "test.txt";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    
    VTL_AppResult result = VTL_SheduleText(test_file, flags);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_SheduleText\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

// Тест для VTL_PubicateText
bool VTL_test_PubicateText(void)
{
    const VTL_Filename test_file = "test.txt";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    
    VTL_AppResult result = VTL_PubicateText(test_file, flags);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_PubicateText\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

// Тест для VTL_ShedulePhoto
bool VTL_test_ShedulePhoto(void)
{
    const VTL_Filename test_file = "test.jpg";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    
    VTL_AppResult result = VTL_ShedulePhoto(test_file, flags);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_ShedulePhoto\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

// Тест для VTL_PubicatePhoto
bool VTL_test_PubicatePhoto(void)
{
    const VTL_Filename test_file = "test.jpg";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    
    VTL_AppResult result = VTL_PubicatePhoto(test_file, flags);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_PubicatePhoto\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

// Тест для VTL_ShedulePhotoWithMarkedText
bool VTL_test_ShedulePhotoWithMarkedText(void)
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
bool VTL_test_PubicatePhotoWithMarkedText(void)
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
bool VTL_test_ShedulePhotoWithText(void)
{
    const VTL_Filename photo_file = "test.jpg";
    const VTL_Filename text_file = "test.txt";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    
    VTL_AppResult result = VTL_ShedulePhotoWithText(photo_file, text_file, flags);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_ShedulePhotoWithText\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

// Тест для VTL_PubicatePhotoWithText
bool VTL_test_PubicatePhotoWithText(void)
{
    const VTL_Filename photo_file = "test.jpg";
    const VTL_Filename text_file = "test.txt";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    
    VTL_AppResult result = VTL_PubicatePhotoWithText(photo_file, text_file, flags);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_PubicatePhotoWithText\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

// Тест для VTL_SheduleVideo
bool VTL_test_SheduleVideo(void)
{
    const VTL_Filename test_file = "test.mp4";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    
    VTL_AppResult result = VTL_SheduleVideo(test_file, flags);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_SheduleVideo\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

// Тест для VTL_PubicateVideo
bool VTL_test_PubicateVideo(void)
{
    const VTL_Filename test_file = "test.mp4";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    
    VTL_AppResult result = VTL_PubicateVideo(test_file, flags);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_PubicateVideo\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

// Тест для VTL_SheduleVideoWithText
bool VTL_test_SheduleVideoWithText(void)
{
    const VTL_Filename video_file = "test.mp4";
    const VTL_Filename text_file = "test.txt";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    
    VTL_AppResult result = VTL_SheduleVideoWithText(video_file, text_file, flags);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_SheduleVideoWithText\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

// Тест для VTL_PubicateVideoWithText
bool VTL_test_PubicateVideoWithText(void)
{
    const VTL_Filename video_file = "test.mp4";
    const VTL_Filename text_file = "test.txt";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    
    VTL_AppResult result = VTL_PubicateVideoWithText(video_file, text_file, flags);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_PubicateVideoWithText\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

// Тест для VTL_SheduleVideoWithMarkedText
bool VTL_test_SheduleVideoWithMarkedText(void)
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
bool VTL_test_PubicateVideoWithMarkedText(void)
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
bool VTL_test_SheduleVideoWithInnerSub(void)
{
    const VTL_Filename video_file = "test.mp4";
    const VTL_Filename sub_file = "test.srt";
    const VTL_content_platform_flags flags = VTL_CONTENT_PLATFORM_TG;
    
    VTL_AppResult result = VTL_SheduleVideoWithInnerSub(video_file, sub_file, flags);
    const char test_fail_message[] = "\nОшибка при тестировании VTL_SheduleVideoWithInnerSub\n";
    return !VTL_test_CheckCondition(result == VTL_res_kOk, test_fail_message);
}

// Тест для VTL_PubicateVideoWithInnerSub
bool VTL_test_PubicateVideoWithInnerSub(void)
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
    int fail_counter = 0;
    
    if (!VTL_test_SheduleMarkedText()) fail_counter++;
    if (!VTL_test_PubicateMarkedText()) fail_counter++;
    if (!VTL_test_SheduleText()) fail_counter++;
    if (!VTL_test_PubicateText()) fail_counter++;
    if (!VTL_test_ShedulePhoto()) fail_counter++;
    if (!VTL_test_PubicatePhoto()) fail_counter++;
    if (!VTL_test_ShedulePhotoWithMarkedText()) fail_counter++;
    if (!VTL_test_PubicatePhotoWithMarkedText()) fail_counter++;
    if (!VTL_test_ShedulePhotoWithText()) fail_counter++;
    if (!VTL_test_PubicatePhotoWithText()) fail_counter++;
    if (!VTL_test_SheduleVideo()) fail_counter++;
    if (!VTL_test_PubicateVideo()) fail_counter++;
    if (!VTL_test_SheduleVideoWithText()) fail_counter++;
    if (!VTL_test_PubicateVideoWithText()) fail_counter++;
    if (!VTL_test_SheduleVideoWithMarkedText()) fail_counter++;
    if (!VTL_test_PubicateVideoWithMarkedText()) fail_counter++;
    if (!VTL_test_SheduleVideoWithInnerSub()) fail_counter++;
    if (!VTL_test_PubicateVideoWithInnerSub()) fail_counter++;
    
    return fail_counter;
} 