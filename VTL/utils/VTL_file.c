#include <VTL/utils/VTL_file.h>

VTL_publication_app_result VTL_publication_FileOpenForReading(VTL_File** pp_file, VTL_Filename file_name)
{
    return VTL_publication_res_kOk;
}

VTL_publication_app_result VTL_publication_FileOpenForWriting(VTL_File** pp_file, VTL_Filename file_name)
{
    return VTL_publication_res_kOk;
}

VTL_publication_app_result VTL_publication_FileReadRawData(VTL_BufferData** buffer_data, const VTL_Filename file_name)
{
    return VTL_publication_res_kOk;
}

VTL_publication_app_result VTL_publication_FileWriteRawData(VTL_BufferData** buffer_data, const VTL_Filename file_name)
{
    return VTL_publication_res_kOk;
}

VTL_publication_app_result VTL_publication_FileCopy(const VTL_Filename out_file_name, const VTL_Filename src_file_name)
{
    return VTL_publication_res_kOk;
}

bool VTL_publication_FileCheckEquality(const VTL_Filename first_file_name, const VTL_Filename second_file_name)
{
    return true;
}