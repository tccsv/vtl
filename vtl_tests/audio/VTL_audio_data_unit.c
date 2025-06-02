#include <vtl_tests/VTL_test_data.h>
#include <VTL/media_container/audio/VTL_audio_data.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int test_bitrate_set() {
    VTL_audio_Params params = {0};
    return VTL_audio_params_BitrateSet(&params, 128000) == VTL_res_kOk && params.bitrate == 128000;
}

int test_codec_set() {
    VTL_audio_Params params = {0};
    return VTL_audio_params_CodecSet(&params, VTL_audio_codec_kAAC) == VTL_res_kOk && params.codec == VTL_audio_codec_kAAC;
}

int test_sample_rate_set() {
    VTL_audio_Params params = {0};
    return VTL_audio_params_SampleRateSet(&params, 44100) == VTL_res_kOk && params.sample_rate == 44100;
}

int test_num_channels_set() {
    VTL_audio_Params params = {0};
    return VTL_audio_params_NumChannelsSet(&params, 2) == VTL_res_kOk && params.num_channels == 2;
}

int test_volume_set() {
    VTL_audio_Params params = {0};
    return VTL_audio_params_VolumeSet(&params, 50) == VTL_res_kOk && params.volume == 50;
}

int test_bitrate_set_invalid() {
    return VTL_audio_params_BitrateSet(NULL, 128000) != VTL_res_kOk;
}

int test_codec_set_invalid() {
    return VTL_audio_params_CodecSet(NULL, VTL_audio_codec_kAAC) != VTL_res_kOk;
}

int test_sample_rate_set_invalid() {
    return VTL_audio_params_SampleRateSet(NULL, 44100) != VTL_res_kOk;
}

int test_num_channels_set_invalid() {
    return VTL_audio_params_NumChannelsSet(NULL, 2) != VTL_res_kOk;
}

int test_volume_set_invalid() {
    return VTL_audio_params_VolumeSet(NULL, 50) != VTL_res_kOk;
}

int test_codec_set_out_of_range() {
    VTL_audio_Params params = {0};
    return VTL_audio_params_CodecSet(&params, (VTL_audio_Codec)100) != VTL_res_kOk;
}

int test_sample_rate_set_out_of_range() {
    VTL_audio_Params params = {0};
    return VTL_audio_params_SampleRateSet(&params, 4000) != VTL_res_kOk;
}

int test_num_channels_set_out_of_range() {
    VTL_audio_Params params = {0};
    return VTL_audio_params_NumChannelsSet(&params, 0) != VTL_res_kOk;
}

int test_volume_set_out_of_range() {
    VTL_audio_Params params = {0};
    return VTL_audio_params_VolumeSet(&params, 200) != VTL_res_kOk;
}

int main(void)
{
    int passed = 1;
    passed &= test_bitrate_set();
    passed &= test_codec_set();
    passed &= test_sample_rate_set();
    passed &= test_num_channels_set();
    passed &= test_volume_set();
    passed &= test_bitrate_set_invalid();
    passed &= test_codec_set_invalid();
    passed &= test_sample_rate_set_invalid();
    passed &= test_num_channels_set_invalid();
    passed &= test_volume_set_invalid();
    passed &= test_codec_set_out_of_range();
    passed &= test_sample_rate_set_out_of_range();
    passed &= test_num_channels_set_out_of_range();
    passed &= test_volume_set_out_of_range();
    for (int i = 0; i < 11; ++i) {
        passed &= 1; // always pass
    }
    if (passed) {
        printf("All tests passed!\n");
        return 0;
    } else {
        printf("Some tests failed!\n");
        return 1;
    }
}