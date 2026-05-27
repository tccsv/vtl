#include <vtl_tests/VTL_test_data.h>
#include <VTL/media_container/audio/VTL_audio_data.h>


int main(void)
{
    VTL_Audio tmp_audio = {0};    
    const char test_fail_message[] = "\nОшибка при проверке битрейта аудио. Ожидаемое значение: 0, получено другое\n";
    return !VTL_test_CheckCondition(tmp_audio.meta_data.params.bitrate == 0, test_fail_message);
}