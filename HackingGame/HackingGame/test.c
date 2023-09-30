#include "../../testing.h/testing.h"

static int sanity_test()
{
    return 1;
}

SUB_TEST(exec_tests, {&sanity_test, "Sanity test"})

int main()
{
    return exec_tests() ? 0 : 1;
}
