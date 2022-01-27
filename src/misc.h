#pragma once
#include <stdint.h>
#define COUNTOF(arr) (sizeof((arr))/sizeof((arr)[0]))

uintptr_t rot_left(uintptr_t n, uint8_t num){
    return (n << num) | (n >> ((sizeof(n) * 8) - num));
}

uintptr_t rot_right(uintptr_t n, uint8_t num){
    return (n >> num) | (n << ((sizeof(n) * 8) - num));
}
