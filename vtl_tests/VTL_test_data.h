#ifndef _VTL_TESTS__H
#define _VTL_TESTS__H

#ifdef __cplusplus
extern "C"
{
#endif


#include <stdio.h>
#include <stdbool.h>

typedef unsigned int VTL_tests_fail_Count; 

void VTL_Print(const char * str);
bool VTL_test_CheckCondition(bool test_condition, const char* test_fail_message);


#ifdef __cplusplus
}
#endif


#endif