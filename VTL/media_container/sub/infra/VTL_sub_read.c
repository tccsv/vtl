#include <VTL/media_container/sub/infra/VTL_sub_read.h>

VTL_AppResult VTL_sub_InitFromASSFile(VTL_Sub** pp_sub, const VTL_Filename ass_file_name)
{
    char* p_raw_data;
    size_t raw_data_size;
    VTL_AppResult app_result = VTL_file_ReadRawData(&p_raw_data, &raw_data_size, ass_file_name);
    if(app_result != VTL_res_kOk)
    {
        return app_result;
    }
    return VTL_sub_srt_Conversion(pp_sub, p_raw_data, raw_data_size);
}

VTL_AppResult VTL_sub_InitFromSRTFile(VTL_Sub** pp_sub, const VTL_Filename srt_file_name)
{
    char* p_raw_data;
    size_t raw_data_size;
    VTL_AppResult app_result = VTL_file_ReadRawData(&p_raw_data, &raw_data_size, srt_file_name);
    if(app_result != VTL_res_kOk)
    {
        return app_result;
    }
    return VTL_sub_srt_Conversion(pp_sub, p_raw_data, raw_data_size);
}

VTL_AppResult VTL_sub_ReadMetaData(VTL_sub_MetaData** pp_sub_meta_data, const VTL_Filename srt_file_name)
{
    return VTL_res_kOk;
}