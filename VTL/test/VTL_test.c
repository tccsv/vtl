#include <VTL/test/VTL_test_data.h>
#include <VTL/media_container/sub/infra/VTL_sub_read.h>

TEST(test_addition) 
{
    int a = 2 + 2;
    VTL_ASSERT(a == 4, "2 + 2 must be equals 4");
}

TEST(test_subtraction) {
    int b = 5 - 3;
    VTL_ASSERT(b == 2, "5 - 3 must be equals 2");
}


int main() 
{
    VTL_RUN_TEST(test_addition);
    VTL_RUN_TEST(test_subtraction);
    return VTL_test_result();
}
