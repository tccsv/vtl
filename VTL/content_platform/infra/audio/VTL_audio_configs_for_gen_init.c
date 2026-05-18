#include <VTL/content_platform/infra/audio/VTL_audio_configs_for_gen_init.h>



VTL_AppResult VTL_audio_configs_for_gen_Init(VTL_audio_Configs** pp_new_configs, 
                                    VTL_audio_configs_platforms_Indices** pp_indices,
                                    const VTL_Filename src_file_name, const VTL_content_platform_flags flags)
{
    VTL_audio_MetaData audio_meta_data = {0};
    VTL_audio_meta_data_InitFromSource(&audio_meta_data, src_file_name);

    *pp_new_configs = (VTL_audio_Configs*) 
                    malloc( VTL_CONTENT_PLATFORM_MAX_NUM * sizeof((*pp_new_configs)->data[0] ) + sizeof(*pp_new_configs) );

    (*pp_new_configs)->length = 0;

    *pp_indices =   (VTL_audio_configs_platforms_Indices*) 
                    malloc(VTL_CONTENT_PLATFORM_MAX_NUM * sizeof((*pp_indices)->data[0]) + sizeof(*pp_indices));
    (*pp_indices)->length = 0;

    VTL_audio_configs_platforms_Indices* p_indices = *pp_indices;
    p_indices->length = 0;

    
    if(VTL_content_platform_flags_CheckTg(flags))
    {        
        VTL_audio_tg_SetParams(&(*pp_new_configs)->data[(*pp_new_configs)->length].params, &audio_meta_data.params);
        ++(*pp_new_configs)->length;

        ++(*pp_indices)->length;
    }
    
    *pp_new_configs = realloc(*pp_new_configs, (*pp_new_configs)->length*sizeof((*pp_new_configs)->data[0]) + sizeof(*pp_new_configs));
    *pp_indices = realloc(*pp_indices, (*pp_indices)->length*sizeof((*pp_indices)->data[0]) + sizeof(*pp_indices));
   
    return VTL_res_kOk;
}