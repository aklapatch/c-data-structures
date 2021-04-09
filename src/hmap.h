#pragma once
#include "dynarr.h"

#ifndef C_DS_HASH_FUNC
// This code is derived from the ahash hash function.
// It's written in rust, so I'm not sure I need to include 
// the license or not since I'm porting it to C
// https://github.com/tkaitchuck/aHash/blob/master/src/fallback_hash.rs
#ifndef C_DS_HASH_SEED1 // pi in hex
#define C_DS_HASH_SEED1 (0x3141592653589793)
#endif

#ifndef C_DS_HASH_SEED2 // e in hex
#define C_DS_HASH_SEED2 (0x2718281828459045)
#endif

#ifndef C_DS_AHASH_MULTIPLE
#define C_DS_AHASH_MULTIPLE (6364136223846793005)
#endif

uint64_t ahash_wrapping_mul(uint64_t a, uint64_t b){
    return (a*b) % UINT64_MAX;
}
uint64_t ahash_wrapping_add(uint64_t a, uint64_t b){
    return (a+b) % UINT64_MAX;
}

uint64_t ahash_rotr(uint64_t n, int32_t c){
    uint32_t mask = (8*sizeof(n) - 1);
    c &= mask;
    return (n << c) | (n>> ( (-c)&mask));
}

uint64_t ahash_rotl(uint64_t n, int32_t c){
    uint32_t mask = (8*sizeof(n) - 1);
    c &= mask;
    return (n >> c) | (n << ( (-c)&mask));
}

void ahash_update(uint64_t * buf, uint64_t * pad, uint64_t data_in){
    uint64_t tmp = ahash_wrapping_mul( (data_in ^ *buf), C_DS_AHASH_MULTIPLE );
    *pad = ahash_wrapping_mul(ahash_rotl((*pad ^ tmp), 8) , C_DS_AHASH_MULTIPLE);
    *buf = ahash_rotl((*buf ^ *pad), 24);
}

void ahash_update_128(uint64_t * buf, uint64_t * pad, uint64_t data_in[2]){
    ahash_update(buf, pad, data_in[0]);
    ahash_update(buf, pad, data_in[1]);
}

uint64_t ahash_buf(uint8_t *data, size_t data_len){
    uint64_t buffer = C_DS_HASH_SEED1, pad = C_DS_HASH_SEED2;

    buffer = ahash_wrapping_mul(C_DS_AHASH_MULTIPLE, ahash_wrapping_add((uint64_t)data_len, buffer));

    if (data_len > 8){
        if (data_len > 16){
            // update on the last 128 bits
            uint64_t last_128[2];
            memcpy(&last_128, &data[data_len - 17], sizeof(last_128));
            ahash_update_128(&buffer, &pad, last_128);
            while (data_len > 16){
                memcpy(last_128, data, sizeof(last_128[0]));
                ahash_update(&buffer, &pad, last_128[0]);
                data += sizeof(uint64_t);
                data_len -= sizeof(uint64_t);
            }
        } else {
            uint64_t first, last;
            memcpy(&first, data, sizeof(first));
            memcpy(&last, data + data_len - 9, sizeof(last));
            ahash_update(&buffer, &pad, first);
            ahash_update(&buffer, &pad, last);
        }
    }else{
        uint64_t vals[2] = {0};
        if (data_len >= 2){
            if (data_len >= 4){
                memcpy(vals, data, 4);
                memcpy(&vals[1], data + data_len - 5, 4);
            } else {
                memcpy(vals, data, 2);
                memcpy(&vals[1], data + data_len - 3, 2);
            }
        } else {
            if (data_len > 0){
                vals[1] = vals[0] = data[0];
            } 
        }
        ahash_update_128(&buffer, &pad, vals);
    }

    uint32_t rot = buffer & 63;
    return ahash_rotl(ahash_wrapping_mul(C_DS_AHASH_MULTIPLE, buffer) ^ pad, rot);
}

#endif

// For the memory layout of the hash table:
// [info struct][data (ptr points here[0])][keys]
// keys are uint64_t for keys
typedef struct {
    uintptr_t cap,count;
    ds_error_e err;
} hm_info;

hm_info * hm_get_info(void * ptr){
    return (ptr == NULL) ? NULL : ((hm_info*)ptr) - 1;
}

uintptr_t hm_cap(void * ptr){
    return (ptr == NULL) ? 0 : hm_get_info(ptr)->cap;
}

uintptr_t hm_count(void * ptr){
    return (ptr == NULL) ? 0 : hm_get_info(ptr)->count;
}

void hm_set_err(void * ptr, ds_error_e err){
    if (ptr != NULL){
        hm_get_info(ptr)->err = err;
    }
}

ds_error_e hm_err(void * ptr){
    if (ptr == NULL){
        return ds_null_ptr;
    } else {
        return hm_get_info(ptr)->err;
    }
}

bool hm_is_err_set(void * ptr){
    return hm_err(ptr) != ds_success;
}

char * hm_err_str(void *ptr){
    return ds_get_err_str(hm_err(ptr));
}
