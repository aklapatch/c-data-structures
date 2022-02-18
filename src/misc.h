#pragma once
#include <stdint.h>
#define COUNTOF(arr) (sizeof((arr))/sizeof((arr)[0]))

uintptr_t rot_left(uintptr_t n, uint8_t num){
    return (n << num) | (n >> ((sizeof(n) * 8) - num));
}

uintptr_t rot_right(uintptr_t n, uint8_t num){
    return (n >> num) | (n << ((sizeof(n) * 8) - num));
}

uintptr_t next_pow2(uintptr_t input){
    input--;
    input |= input >> 1;
    input |= input >> 2;
    input |= input >> 4;
    input |= input >> 8;
    input |= input >> 16;
    input |= input >> 32;
    input++;
    return input;
}
