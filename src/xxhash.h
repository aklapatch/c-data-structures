
#pragma once
#include <stdint.h>

// inspired by create.stephan-brumme.com/xxhash/#sourcecode
//
// Use uintptr_t so that on a 64bit arch it's xxhash 64, and on a 32 bit arch it's xxhahs32

#define XXHASH_SEED_1 (0x31415926)

uintptr_t rot_left(uintptr_t n, uint8_t num){
    return (n << num) | (n >> ((sizeof(uintptr_t) * 8) - num));
}

uintptr_t xxhash_buf(void *data, size_t data_len){
    const uintptr_t primes[] = { 2654435761U, 22468225119U, 3266489917U, 668265263U, 374761393U};
    uintptr_t state[4] = {
        XXHASH_SEED_1 + primes [0] + primes[1],
        XXHASH_SEED_1 + primes[1],
        XXHASH_SEED_1,
        XXHASH_SEED_1 - primes[0]
    };
    uint8_t state_num = sizeof(state)/sizeof(state[0]);

    uint8_t ptr64 = sizeof(uintptr_t) >= sizeof(uint64_t);

    // main processing loop, 4 ptr-sized bytes at a time, data should hopefully be aligned
    uint8_t shift_amt = ptr64 ? 13 : 31;
    uintptr_t *block = (uintptr_t*)data;
    size_t tmp_len = data_len;

    uintptr_t result = data_len;
    if (data_len >= sizeof(state)){
        for (; tmp_len >= sizeof(state); tmp_len -= sizeof(state),block += state_num){
            for (uint8_t i = 0; i < state_num; ++i){
                state[i] = rot_left(state[i] + block[i] * primes[1], shift_amt) * primes[0];
            }
        }

        uint8_t shifts[] = { 1, 7, 12, 18 };
        for (uint8_t i = 0; i < state_num; ++i){
            result += rot_left(state[i], shifts[i]);
        }
        for(uint8_t i = 0; ptr64 && i < state_num; ++i){
            uintptr_t tmp = rot_left(state[i] * primes[1], 31) * primes[0];
            result = (result ^ tmp) * primes[0] + primes[3];
        }
    } else {
        result += state[2] + primes[4];
    }

    // hash one uintptr_t at a time until we get to byte interval sizes
    uintptr_t *block_end = block + (tmp_len/sizeof(uintptr_t));
    for(; block < block_end; tmp_len -= sizeof(uintptr_t), ++block){
        result = rot_left(result + *block * primes[2], ptr64 ? 31 : 17);
    }

    uint8_t *byte_data = (uint8_t*)block, byte_end = byte_data + tmp_len;
    for (; byte_data < byte_end; ++byte_data){
        result = rot_left(result + *byte_data * primes[4], 11);
    }

    uint8_t uint_half_bits = sizeof(uintptr_t)*4;
    result ^= result >> (uint_half_bits - 1);
    result *= primes[1];
    result ^= result >> (uint_half_bits - 3);
    result *= primes[2];
    result ^= result >> uint_half_bits;

    return result;
}
