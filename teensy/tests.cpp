#include <stdio.h>
#include "tests.h"

static int8_t test1(void)
{
    ASSERT((0x100 * 0x200) >> 12 == 0x20);
    ASSERT((0x1000 * 0x2000) >> 12 == 0x2000);
    ASSERT((0x1000 * -0x2000) >> 12 == -0x2000);
    ASSERT((-0x1000 * 0x2000) >> 12 == -0x2000);
    ASSERT((-0x1000 * -0x2000) >> 12 == 0x2000);
    return 0;
}

static int8_t test2(void)
{
    return 0;
}

static int8_t test3(void)
{
    return 0;
}

int8_t run_tests(void)
{
    return !(
        test1() == 0 &&
        test2() == 0 &&
        test3() == 0
    );
}
