#ifndef _VTL_APP_RESULT_H
#define _VTL_APP_RESULT_H

#ifdef __cplusplus
extern "C"
{
#endif



typedef enum _VTL_publication_app_result 
{
    VTL_publication_res_kOk = 0,
    
    
    VTL_publication_res_kMissingFileErr = 1,
    VTL_publication_res_kFileIsBusyErr,

    VTL_publication_res_kFileIsBusyErr = 10,


    
} VTL_publication_app_result;




#ifdef __cplusplus
}
#endif


#endif