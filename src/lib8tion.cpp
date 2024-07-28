/// @file lib8tion.cpp
/// Fast, efficient 8-bit math functions specifically
/// designed for high-performance LED programming. 

/// Disables pragma messages and warnings
#define FASTLED_INTERNAL

#include <stdint.h>
#include "FastLED_Lite.h"

FASTLED_NAMESPACE_BEGIN

/// @copydoc ::rand16seed
#define RAND16_SEED  1337
uint16_t rand16seed = RAND16_SEED;



FASTLED_NAMESPACE_END