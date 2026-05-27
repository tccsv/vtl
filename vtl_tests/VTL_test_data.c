#include <VTL_tests/VTL_test_data.h>


void VTL_Print(const char * str)
{
    printf(str);
}

bool VTL_test_CheckCondition(bool test_condition, const char* test_fail_message)
{
    if(!test_condition)
    {
        VTL_Print(test_fail_message);
    }
    return test_condition;
}