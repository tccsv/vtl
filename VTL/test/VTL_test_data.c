#include <VTL/test/VTL_test_data.h>
#include <VTL/media_container/sub/infra/VTL_sub_read.h>

unsigned int VTL_global_test_fails = 0;

int VTL_test_result(void) 
{
    if (VTL_global_test_fails == 0) 
    {
        printf(VTL_RUN_TESTS_PASSED"\n");
        return 0; 
    } 
    else {
        printf(VTL_RUN_TESTS_FAILS_FORMAT, VTL_global_test_fails);
        return 1; 
    }
}