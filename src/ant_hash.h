#pragma once
#include "misc.h"
#include <stdint.h> // if you're not using stdint, you're missing out

// The hash table that takes keys only really needs to hash one uintptr_t at a time.
// I can Probably make a hash function that only hashes one uintptr_t at a time that is 
// faster than other more generic hash functions since those more generic hash 
// functions have different branches to handle alignment, and we don't need those

// The more architecture portable version. A uintptr_t will be change between 
// 32 and 64 bits as needed
// Tried multiplying by the golden ration and that did not help. Primes work better.
// Played with the shift amt and 8*2 for 64 bit ptrs seems to work well enough.
// Doing the 2nd bit mix seems to work well enough too
// multiplying by the same value twice seems to help speed, probably since the value can be
// saved in the same register
uintptr_t ant_hash(uintptr_t in){
    const uint8_t shift_amt = sizeof(in)*3;
    const uintptr_t prime1 = 22468225119U;
    in ^= in >> shift_amt;
    in ^= in >> shift_amt;
    in ^= prime1;
    return in * prime1 * prime1;
}
