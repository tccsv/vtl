#include <VTL/content_platform/infra/audio/VTL_audio_gen.h>


static VTL_AppResult VTL_audio_GenPart(VTL_audio_Data** pp_audio_part, const VTL_audio_Params* p_src_params, 
                                            const VTL_audio_Congif *p_config, VTL_audio_File* p_output)
{
    VTL_audio_Data_Encode(pp_audio_part, p_src_params, &p_config->params);
    VTL_audio_WritePart(*pp_audio_part, p_output); 
    return VTL_res_kOk;
}

static VTL_AppResult VTL_audio_GenParts(const VTL_Audio* p_audio, const VTL_audio_Configs *p_configs, 
                                            VTL_audio_File** pp_outputs)
{
    VTL_audio_Data* p_audio_part = (VTL_audio_Data*) malloc(sizeof(*p_audio->p_current_part));  
    memcpy(p_audio_part, p_audio->p_current_part, sizeof(*p_audio->p_current_part));
    for(size_t i = 0; i < p_configs->length; ++i)
    {
        VTL_audio_GenPart(&p_audio_part, &p_audio->meta_data.params, &p_configs->data[i], pp_outputs[i]);
    }
    return VTL_res_kOk;
}

static VTL_AppResult VTL_audio_OpenOutputs(VTL_audio_File*** ppp_outputs, const VTL_audio_Configs* p_configs)
{
    const size_t outputs_length = p_configs->length;
    *ppp_outputs = (VTL_audio_File**) malloc(outputs_length * sizeof(VTL_audio_File));
    VTL_audio_File* p_output = {0}; 
    for(size_t i = 0; i < outputs_length; ++i)
    {
        
        VTL_audio_OpenOutput(&p_output, p_configs->data[i].file_name);
    }

    return VTL_res_kOk;
}

static VTL_AppResult VTL_audio_CloseOutputs(VTL_audio_File** outputs, const size_t outputs_length)
{
    for(size_t i = 0; i < outputs_length; ++i)
    {
        VTL_audio_CloseOutput(outputs[i]);
    }

    return VTL_res_kOk;
}

VTL_AppResult VTL_audio_Gen(const VTL_Filename file_name, const VTL_audio_Configs* p_configs)
{
    
    VTL_Audio* p_audio = (VTL_Audio*) malloc(VTL_BUFFER_DATA_SIZE_DEFAULT + sizeof(VTL_Audio));
    VTL_audio_File* p_audio_r_file = {0};
    VTL_audio_file_op_Result audio_r_op_res;
   
    VTL_audio_read_OpenSource(&p_audio_r_file, file_name);
    
    VTL_audio_ReadMetaData(&p_audio->meta_data, p_audio_r_file, &audio_r_op_res);

    VTL_audio_File** pp_outputs;
    const size_t outputs_length = p_configs->length;
    VTL_audio_OpenOutputs(&pp_outputs, p_configs);
    while(audio_r_op_res != VTL_AUDIO_FILE_EOF)
    {        
        VTL_audio_ReadPart(&p_audio->p_current_part, &p_audio->meta_data, p_audio_r_file, &audio_r_op_res);
        VTL_audio_GenParts(p_audio, p_configs, pp_outputs);
        audio_r_op_res = VTL_AUDIO_FILE_EOF;
    }

    VTL_audio_read_CloseSource(p_audio_r_file);
    VTL_audio_CloseOutputs(pp_outputs, outputs_length);
    
    return VTL_res_kOk;
}
