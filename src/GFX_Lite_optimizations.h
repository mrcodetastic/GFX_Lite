#ifndef _GFX_LITE_OPTIMIZATIONS_H_
#define _GFX_LITE_OPTIMIZATIONS_H_

/**
 * @file GFX_Lite_optimizations.h
 * @brief Platform-specific optimizations and compiler hints for GFX_Lite
 */

// Compiler optimization hints
#ifdef __GNUC__
  #define GFX_LIKELY(x)       __builtin_expect(!!(x), 1)
  #define GFX_UNLIKELY(x)     __builtin_expect(!!(x), 0)
  #define GFX_FORCE_INLINE    __attribute__((always_inline)) inline
  #define GFX_NO_INLINE       __attribute__((noinline))
  #define GFX_PURE            __attribute__((pure))
  #define GFX_CONST           __attribute__((const))
#else
  #define GFX_LIKELY(x)       (x)
  #define GFX_UNLIKELY(x)     (x)
  #define GFX_FORCE_INLINE    inline
  #define GFX_NO_INLINE
  #define GFX_PURE
  #define GFX_CONST
#endif

// Memory alignment hints
#ifdef __GNUC__
  #define GFX_ALIGN(x)        __attribute__((aligned(x)))
  #define GFX_PACKED          __attribute__((packed))
#else
  #define GFX_ALIGN(x)
  #define GFX_PACKED
#endif

// Fast math operations for ESP32/ARM platforms
#if defined(ESP32) || defined(__ARM_ARCH)
  #define GFX_FAST_MATH_AVAILABLE 1
  
  // Fast integer square root for distance calculations
  GFX_FORCE_INLINE uint16_t fast_sqrt(uint32_t x) {
    if (x == 0) return 0;
    
    uint32_t root = 0;
    uint32_t bit = 1UL << 30; // Second-highest bit
    
    while (bit > x) bit >>= 2;
    
    while (bit != 0) {
      if (x >= root + bit) {
        x -= root + bit;
        root = (root >> 1) + bit;
      } else {
        root >>= 1;
      }
      bit >>= 2;
    }
    return (uint16_t)root;
  }
  
  // Fast color interpolation using bit shifts
  GFX_FORCE_INLINE uint8_t fast_lerp8(uint8_t a, uint8_t b, uint8_t frac) {
    return a + (((b - a) * frac) >> 8);
  }
#else
  #define GFX_FAST_MATH_AVAILABLE 0
#endif

// Cache optimization hints
#define GFX_CACHE_LINE_SIZE 32
#define GFX_CACHE_ALIGN     GFX_ALIGN(GFX_CACHE_LINE_SIZE)

// Loop unrolling hints
#ifdef __GNUC__
  #define GFX_UNROLL_LOOP(n)  _Pragma("GCC unroll " #n)
#else
  #define GFX_UNROLL_LOOP(n)
#endif

// Prefetch hints for better cache performance
#ifdef __GNUC__
  #define GFX_PREFETCH_READ(addr)   __builtin_prefetch(addr, 0, 3)
  #define GFX_PREFETCH_WRITE(addr)  __builtin_prefetch(addr, 1, 3)
#else
  #define GFX_PREFETCH_READ(addr)
  #define GFX_PREFETCH_WRITE(addr)
#endif

// Platform-specific optimizations
#ifdef ESP32
  // ESP32 has hardware floating point
  #define GFX_USE_FLOAT_MATH 1
  
  // ESP32 dual-core optimization flags
  #define GFX_DUAL_CORE_AVAILABLE 1
#else
  #define GFX_USE_FLOAT_MATH 0
  #define GFX_DUAL_CORE_AVAILABLE 0
#endif

// Memory management optimizations
#define GFX_SMALL_MEMORY_DEVICE (defined(__AVR__) || defined(ESP8266))

#if GFX_SMALL_MEMORY_DEVICE
  // Use smaller buffers and more conservative allocation on limited devices
  #define GFX_MAX_LAYER_SIZE 16384  // 128x128 pixels max
  #define GFX_USE_PROGMEM_TABLES 1
#else
  #define GFX_MAX_LAYER_SIZE 262144 // 512x512 pixels max
  #define GFX_USE_PROGMEM_TABLES 0
#endif

#endif // _GFX_LITE_OPTIMIZATIONS_H_
