#include <VTL/content_platform/infra/audio/VTL_content_platform_infra_audio_configs_for_gen_init.h>
#include <string.h>



VTL_AppResult VTL_audio_configs_for_gen_Init(VTL_audio_Configs** pp_new_configs, 
                                    VTL_audio_configs_platforms_Indices** pp_indices,
                                    const VTL_Filename src_file_name, const VTL_content_platform_flags flags)
{
    VTL_audio_MetaData audio_meta_data = {0};
    VTL_audio_meta_data_InitFromSource(&audio_meta_data, src_file_name);

    *pp_new_configs = (VTL_audio_Configs*)
                    malloc( sizeof(VTL_audio_Configs) + VTL_CONTENT_PLATFORM_MAX_NUM * sizeof(VTL_audio_Congif) );
    (*pp_new_configs)->data = (VTL_audio_Congif*)((char*)(*pp_new_configs) + sizeof(VTL_audio_Configs));
    (*pp_new_configs)->length = 0;

    *pp_indices =   (VTL_audio_configs_platforms_Indices*)
                    malloc(sizeof(VTL_audio_configs_platforms_Indices) + VTL_CONTENT_PLATFORM_MAX_NUM * sizeof(VTL_ContentPlatform));
    (*pp_indices)->data = (VTL_ContentPlatform*)((char*)(*pp_indices) + sizeof(VTL_audio_configs_platforms_Indices));
    (*pp_indices)->length = 0;

    VTL_audio_configs_platforms_Indices* p_indices = *pp_indices;
    p_indices->length = 0;

    
    if(VTL_content_platform_flags_CheckTg(flags))
    {
        size_t idx = (*pp_new_configs)->length;
        VTL_audio_tg_SetParams(&(*pp_new_configs)->data[idx].params, &audio_meta_data.params);
        strncpy((*pp_new_configs)->data[idx].file_name, src_file_name, sizeof((*pp_new_configs)->data[idx].file_name) - 1);
        ++(*pp_new_configs)->length;

        p_indices->data[p_indices->length] = VTL_CONTENT_PLATFORM_TG;
        ++p_indices->length;
    }
    
    /* realloc omitted: MAX_NUM is small and realloc would invalidate data pointers */
   
    return VTL_res_kOk;
}