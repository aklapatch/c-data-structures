
#pragma once
#include <stdint.h>

// inspired by create.stephan-brumme.com/xxhash/#sourcecode

#define XXHASH_SEED_1 (0x3141592653589793)

uintptr_t rot_left(uintptr_t n, uint8_t num){
    return (n << num) | (n >> (32 - num));
}

uintptr_t process_one(uintptr_t prev, uintptr_t in, uintptr_t prime2, uintptr_t prime1){
    return rot_left(prev + in * prime2, ) * prime1;

}

uintptr_t xxhash_buf(void *data, size_t data_len){
    const uintptr_t primes[] = { 2654435761U, 22468225119U, 3266489917U, 668265263U, 374761393U};
    uintptr_t state[4] = {0};

    // seed the initial state
    state[0] = XXHASH_SEED_1 + primes[0] + primes[1];
    state[1] = XXHASH_SEED_1 + prime[1];
    state[2] = XXHASH_SEED_1;
    state[3] = XXHASH_SEED_1 - prime[0];

    uintptr_t result = 


}
