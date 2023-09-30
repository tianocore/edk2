#include "../../testing.h/testing.h"
#include "./test_words.h"

static int sanity_test()
{
    return 1;
}

SUB_TEST(exec_tests, {&sanity_test, "Sanity test"},
{&test_words, "Test words"})

int main()
{
    return exec_tests() ? 0 : 1;
}
