#pragma once
#include <stdint.h>

// These functions should not replace any optimized memcpy or memset.
// These will be more useful in an embedded or less optimized
// platform where the vendor has not provided an optimized memcpy
// (looking at you xilinx)

    // half-word mask
#define HW_MASK (sizeof(uint16_t) - 1)
    // 'word' mask
#define W_MASK (sizeof(uint32_t) - 1)

// This function is for benchmarking purposes.
void *naive_memcpy(void *dst, const void *src, size_t n){
    const uint8_t *src8 = src, *src_end = src8 + n;
    uint8_t *dst8 = dst;
    for (; src8 < src_end; ++src8,++dst8){ *dst8 = *src8; }
    return dst;
}

// This function is for benchmarking purposes.
void *naive_memset(void *dst, int val, size_t n){
    uint8_t *dst8 = dst, *dst_end = dst8 + n;
    uint8_t val8 = (uint8_t)val;
    for (; dst8 < dst_end; ++dst8){ *dst8 = val8; }
    return dst;
}

void *mo_memcpy(void *dst, const void *src, size_t n){
    const uint8_t *src8 = src, *src_end = src8 + n;
    uint8_t *dst8 = dst;

    // go up till a 2 byte boundary
    for (; src8 < src_end && ((uintptr_t)src8 & HW_MASK) != 0; ++src8,++dst8){ *dst8 = *src8; }

    // if they are 2 byte aligned, use 2 byte copy
    if (((uintptr_t)src8 & HW_MASK) == ((uintptr_t)dst8 & HW_MASK)){
        size_t len16 = (size_t)(src_end - src8)/2;
        const uint16_t *src16 = (uint16_t*)src8, *end16 = src16 + len16;
        uint16_t *dst16 = (uint16_t*)dst8;

        for (; src16 < end16 && ((uintptr_t)src16 & W_MASK) != 0; ++src16,++dst16){ *dst16 = *src16; }

        if (((uintptr_t)src16 & W_MASK) == ((uintptr_t)dst16 & W_MASK)){
            size_t len32 = (size_t)(end16 - src16)/2;
            const uint32_t *src32 = (uint32_t*)src16, *end32 = src32 + len32;
            uint32_t *dst32 = (uint32_t*)dst16;

            for (; src32 < end32; ++src32,++dst32){ *dst32 = *src32; }

            // sync back up
            src16 = (uint16_t*)src32; dst16 = (uint16_t*)dst32;
        }

        for (; src16 < end16; ++src16,++dst16){ *dst16 = *src16; }

        // sync back up
        src8 = (uint8_t*)src16; dst8 = (uint8_t*)dst16;
    }

    // finish up
    for (; src8 < src_end; ++src8,++dst8){ *dst8 = *src8; }

    return dst;
}

void *mo_memset(void *dst, int val, size_t n){
    uint8_t *dst8 = dst, *end = dst8 + n;
    uint8_t val8 = (uint8_t)val;

    for (; dst8 < end && ((uintptr_t)dst8 & HW_MASK) != 0; ++dst8){ *dst8 = val8; }

    size_t len16 = (size_t)(end - dst8)/2;
    uint16_t *dst16 = (uint16_t*)dst8, *end16 = dst16 + len16;
    uint16_t val16 = val8 * 0x0101;
    for (; dst16 < end16 && ((uintptr_t)dst16 & W_MASK) != 0; ++dst16){ *dst16 = val16; }

    size_t len32 = (size_t)(end - (uint8_t*)dst16)/4;
    uint32_t *dst32 = (uint32_t*)dst16, *end32 = dst32 + len32;
    uint32_t val32 = val16 * 0x00010001;

    for (; dst32 < end32; ++dst32){ *dst32 = val32; }

    // sync up
    dst16 = (uint16_t*)dst32;

    for (; dst16 < end16; ++dst16){ *dst16 = val16; }

    // sync up
    dst8 = (uint8_t*)dst16;

    // clean up
    for (; dst8 < end; ++dst8){ *dst8 = val8; }

    return dst;
}
#undef W_MASK
#undef HW_MASK
