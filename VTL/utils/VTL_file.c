#include <VTL/utils/VTL_file.h>

VTL_AppResult VTL_file_OpenForReading(VTL_File** pp_file, VTL_Filename file_name)
{
    return VTL_res_kOk;
}

VTL_AppResult VTL_file_OpenForWriting(VTL_File** pp_file, VTL_Filename file_name)
{
    return VTL_res_kOk;
}

VTL_AppResult VTL_file_ReadRawData(VTL_BufferData** buffer_data, const VTL_Filename file_name)
{
    return VTL_res_kOk;
}

VTL_AppResult VTL_file_WriteRawData(VTL_BufferData** buffer_data, const VTL_Filename file_name)
{
    return VTL_res_kOk;
}

VTL_AppResult VTL_file_Copy(const VTL_Filename out_file_name, const VTL_Filename src_file_name)
{
    return VTL_res_kOk;
}


bool VTL_file_CheckEquality(const VTL_Filename first_file_name, const VTL_Filename second_file_name)
{
    return true;
}