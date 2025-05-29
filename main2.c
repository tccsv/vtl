#include <VTL/VTL.h>


int main(void)
{
    VTL_AppResult app_result = VTL_PubicateAudioWithMarkedText("audio0.mp3", "text.md", VTL_markup_type_kTelegramMD, VTL_CONTENT_PLATFORM_W | VTL_CONTENT_PLATFORM_TG);
    VTL_console_out_PotencialErr(app_result);
    return app_result;
}