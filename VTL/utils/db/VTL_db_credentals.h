#ifndef _VTL_DB_CREDENTALS_H
#define _VTL_DB_CREDENTALS_H

#ifdef __cplusplus
extern "C"
{
#endif


#include <VTL/VTL_app_result.h>

typedef struct _VTL_db_Credentals
{
    int x;
} VTL_db_Credentals;


VTL_AppResult VTL_db_credentals_Zeroize(VTL_db_Credentals* p_credentals);


#ifdef __cplusplus
}
#endif


#endif