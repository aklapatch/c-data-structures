#pragma once
#include <stdint.h> // if you're not using stdint, you're missing out
#include <string.h> // for memcpy

#ifndef ANT_USIZE_SEED
// use a 32bit seed to be safe for 32bit arches
// This is 2^32 divided by the golden ration
#define ANT_USIZE_SEED (2654435771)
#endif


// This hash function is designed around a particular use case.
// - All the data you need to hash is present at once (no peacemeal hashing, no update())
// - The data is relatively small (like a pointer sized amount). Small enough where 
//   keeping 16 or 32 bytes of state around is pointless.
//   FxHasher is a good example of a good function for this. I first read about FxHasher
//   here: https://nnethercote.github.io/2021/12/08/a-brutally-effective-hash-function-in-rust.html
// - You don't need an endian aware hash.

uintptr_t ant_rot_left(uintptr_t n, uint8_t num){
    return (n << num) | (n >> ((sizeof(uintptr_t) * 8) - num));
}

uintptr_t ant_usize_process(uintptr_t state, uintptr_t in){
    // rotate by half the pointer size
    state = ant_rot_left(state, sizeof(uintptr_t)*4);
    return state ^ in;
}

// The more architecture portable version. A uintptr_t will be change between 
// 32 and 64 bits as needed
uintptr_t ant_hash(void *data, uintptr_t len){

#define PTR_MASK (sizeof(uintptr_t) - 1)

    uintptr_t state = ANT_USIZE_SEED + len;
    if (len == 0){ return state; }

    uint8_t *data8 = (uint8_t*)data, *data_end = data8 + len;
    if ((PTR_MASK & (uintptr_t)data) == 0){
        // aligned case
        uintptr_t *data_usize = (uintptr_t*)data, *usize_end = data_usize + (len/sizeof(uintptr_t));

        // hash until we can't hash a usize amount of data
        for (; data_usize < usize_end; ++data_usize){
            state = ant_usize_process(state, data_usize[0]);
        }

        // copy the rest into a uint and process it
        if ((uint8_t*)data_usize < data_end){
            uintptr_t last = 0, end_len = data_end - (uint8_t*)data_usize;
            memcpy(&last, data_usize, end_len);
            state = ant_usize_process(state, last);
        }
        return state;
    }

    // if we're not aligned (sad day), then memcpy into a buffer for processing
    // make this buffer about a cache line long
    for (uintptr_t buf_len; data8 < data_end; data8 += buf_len){
        // I really don't want to memset this, so it's going inside this loop to zero it
        uintptr_t buf[64/sizeof(uintptr_t)] = {0};

        buf_len = data_end - data8;
        buf_len = (buf_len > sizeof(buf)) ? sizeof(buf) : buf_len;

        memcpy(buf, data8, buf_len);

        // round up to the next time needed
        for (uint8_t i = 0, times = (buf_len + (sizeof(uintptr_t) - 1))/sizeof(uintptr_t);
                i < times; ++i){
            state = ant_usize_process(state, buf[i]);
        }
    }
    return state;
}
