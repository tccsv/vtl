#ifndef _VTL_APP_RESULT_H
#define _VTL_APP_RESULT_H

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum _VTL_AppResult 
{
    VTL_publication_res_kOk = 0,
    VTL_publication_res_kMissingFileErr = 1,
    VTL_publication_res_kFileIsBusyErr = 10,
    VTL_publication_res_kTextInitErr = 100,
    VTL_publication_res_kTextTransformErr = 101,
    VTL_publication_res_kTextMarkupErr = 102,
    VTL_publication_res_kTextFormatErr = 103,
    
} VTL_AppResult;

#ifdef __cplusplus
}
#endif

#endif
