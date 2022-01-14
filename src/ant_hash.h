#pragma once
#include <stdint.h> // if you're not using stdint, you're missing out

// The hash table that takes keys only really needs to hash one uintptr_t at a time.
// I can Probably make a hash function that only hashes one uintptr_t at a time that is 
// faster than other more generic hash functions since those more generic hash 
// functions have different branches to handle alignment, and we don't need those

uintptr_t ant_rot_left(uintptr_t n, uint8_t num){
    return (n << num) | (n >> ((sizeof(n) * 8) - num));
}

// The more architecture portable version. A uintptr_t will be change between 
// 32 and 64 bits as needed
uintptr_t ant_hash(uintptr_t in){
    const uintptr_t prime1 = 22468225119U;
    in ^= ant_rot_left(in, sizeof(in)*4);
    return in * prime1;
}
