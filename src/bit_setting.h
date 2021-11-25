#pragma once
#include <stdint.h>
#include <stdbool.h>

// These functions are not endian-aware, please be careful using them on different uarches!

// set a bit if true, clear it otherwise
void bit_set_or_clear(void *data, uintptr_t bit_to_set, bool value){
    uint8_t *byte_ptr = (uint8_t*)data;
    uintptr_t byte_to_set = bit_to_set/8;
    bit_to_set -= (byte_to_set*8);

    if (value){
        byte_ptr[byte_to_set] |= (uint8_t)(1 << bit_to_set);
    } else {
        byte_ptr[byte_to_set] &= ~(uint8_t)(1 << bit_to_set);
    }
}

bool bit_get(void *data, uintptr_t bit_to_get){
    uint8_t *byte_ptr = (uint8_t*)data;
    uintptr_t byte_to_get = bit_to_get/8;
    bit_to_get -= (byte_to_get*8);

    return (byte_ptr[byte_to_get] & (1 << bit_to_get))  > 0 ? true : false;
}
