#pragma once

uint8_t highest_set_bit(uint8_t n){
    n |= (n >> 1);
    n |= (n >> 2);
    n |= (n >> 4);
    return n - (n >> 1);
}

uint8_t highest_set_bit_i(uint8_t input){
    uint8_t top_bit_set = highest_set_bit(input);
    uint8_t ret = 0;
    if (top_bit_set == 0) return 0;

    for (; (top_bit_set & 1) == 0; top_bit_set>>=1,++ret){ }

    return ret;
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
