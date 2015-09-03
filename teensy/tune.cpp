#include <stdint.h>

uint32_t tune[] = {
    0, 1, 50, 0,        // keydown major C chord
    300, 1, 54, 0,
    600, 1, 57, 0,
    900, 1, 62, 0,
    2000, 2, 50, 0,     // keyup
    2000, 2, 54, 0,
    2000, 2, 57, 0,
    2000, 2, 62, 0,

    2500, 1, 47, 0,     // keydown IV chord
    2800, 1, 50, 0,
    3100, 1, 55, 0,
    3400, 1, 59, 0,
    3700, 1, 62, 0,
    4500, 2, 47, 0,     // keyup
    4500, 2, 50, 0,
    4500, 2, 55, 0,
    4500, 2, 59, 0,
    4500, 2, 62, 0,

    5000, 1, 49, 0,     // keydown V chord
    5300, 1, 52, 0,
    5600, 1, 57, 0,
    5900, 1, 61, 0,
    6200, 1, 64, 0,
    7000, 2, 49, 0,     // keyup
    7000, 2, 52, 0,
    7000, 2, 57, 0,
    7000, 2, 61, 0,
    7000, 2, 64, 0,

    7500, 1, 50, 0,     // keydown I chord
    7800, 1, 54, 0,
    8100, 1, 57, 0,
    8400, 1, 62, 0,
    8700, 1, 66, 0,
    9500, 2, 50, 0,     // keyup
    9500, 2, 54, 0,
    9500, 2, 57, 0,
    9500, 2, 62, 0,
    9500, 2, 66, 0,

    10000, 0, 0, 0     // end of tune
};