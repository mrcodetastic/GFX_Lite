#ifndef __INC_FASTLED_CONFIG_H
#define __INC_FASTLED_CONFIG_H

/// @def FASTLED_NOISE_ALLOW_AVERAGE_TO_OVERFLOW
/// Use this to allow some integer overflows/underflows in the inoise() functions.
/// The original implementions allowed this, and had some discontinuties in the noise
/// output.  It's technically an implementation bug, and was fixed, but you may wish
/// to preserve the old look and feel of the inoise() functions in your existing animations.  
/// The default is 0:  NO overflow, and 'continuous' noise output, aka the fixed way.
// #define FASTLED_NOISE_ALLOW_AVERAGE_TO_OVERFLOW 0
// #define FASTLED_NOISE_ALLOW_AVERAGE_TO_OVERFLOW 1

/// @def FASTLED_SCALE8_FIXED
/// Use this to toggle whether or not to use the "fixed" FastLED scale8().  The initial scale8()
/// had a problem where scale8(255,255) would give you 254.  This is now fixed, and that
/// fix is enabled by default.  However, if for some reason you have code that is not
/// working right as a result of this (e.g. code that was expecting the old scale8() behavior)
/// you can disable it here.
#define FASTLED_SCALE8_FIXED 1
// #define FASTLED_SCALE8_FIXED 0

/// @def FASTLED_BLEND_FIXED
/// Use this to toggle whether to use "fixed" FastLED pixel blending, including ColorFromPalette.
/// The prior pixel blend functions had integer-rounding math errors that led to
/// small errors being inadvertently added to the low bits of blended colors, including colors
/// retrieved from color palettes using LINEAR_BLEND.  This is now fixed, and the
/// fix is enabled by default.  However, if for some reason you wish to run with the old
/// blending, including the integer rounding and color errors, you can disable the bugfix here.
#define FASTLED_BLEND_FIXED 1
// #define FASTLED_BLEND_FIXED 0

/// @def FASTLED_NOISE_FIXED
/// Use this to toggle whether to use "fixed" FastLED 8-bit and 16-bit noise functions.
/// The prior noise functions had some math errors that led to "discontinuities" in the
/// output, which by definition should be smooth and continuous.  The bug led to 
/// noise function output that had "edges" and glitches in it.  This is now fixed, and the
/// fix is enabled by default.  However, if for some reason you wish to run with the old
/// noise code, including the glitches, you can disable the bugfix here.
#define FASTLED_NOISE_FIXED 1
//#define FASTLED_NOISE_FIXED 0



#endif
