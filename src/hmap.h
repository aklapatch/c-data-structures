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

#define C_DS_HASH_FUNC ahash_buf

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

#define GROUP_SIZE (16)
typedef struct {
    uint8_t meta[GROUP_SIZE];
    uint64_t keys[GROUP_SIZE];
} key_meta_group;

// API:
// - find (key) -> index
// - set (key, val) -> err
// - get (key) -> val
// - set_cap

// For the memory layout of the hash table:
// [info struct][data (ptr points here[0])][keys]
// keys are uint64_t for keys
c_ds_info * hm_info_ptr(void * ptr){
    return dynarr_get_info(ptr);
}

uintptr_t hm_cap(void * ptr){
    return dynarr_cap(ptr);
}

uintptr_t hm_num(void * ptr){
    return dynarr_len(ptr);
}

void hm_set_err(void * ptr, ds_error_e err){
    dynarr_set_err(ptr, err);
}

ds_error_e hm_err(void * ptr){
    return dynarr_err(ptr);
}

bool hm_is_err_set(void * ptr){
    return dynarr_is_err_set(ptr);
}

char * hm_err_str(void *ptr){
    return ds_get_err_str(hm_err(ptr));
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

uintptr_t truncate_to_cap(uintptr_t num, void* ptr){
    return num & (hm_cap(ptr) - 1);
}

bool hmap_storage_bit_set(uint8_t num){
    return (num >= 0b10000000);
}

uintptr_t jump_distance(uint8_t i){
    // truncate to lower 7 bits
    i &= 0x7f;
    uintptr_t tri_input = (i - 10)*(i > 15);
    uintptr_t i_minus = (i - 81)*(i >= 82);
    tri_input += (i_minus + i_minus*i_minus + i_minus*i_minus*i_minus);
    uintptr_t tri_out = (tri_input*(tri_input+1)) >> 1;
    return i*(i < 16) + tri_out;
}

key_meta_group* hm_bare_meta_ptr(void * ptr, size_t item_size){
    if (ptr == NULL){
        return NULL;
    }
    uintptr_t size_to_pass = item_size * hm_cap(ptr);
    key_meta_group * meta_ptr = (uint8_t*)ptr + size_to_pass;
    return meta_ptr;
}

#define hm_meta_ptr(ptr) hm_bare_meta_ptr(ptr, sizeof(*(ptr)))

// This returns UINTPTR_MAX upon failure
// This table uses jump distances to put other items into the array of keys and items
// Theh top bit is set if a matching hash is there, the rest of the 7 bits are the jump distance
// if the jump distance is 0 and the  key does not match, then there are no more items matching that
// hash, if the top bit of the byte array is set, then the slot is being used for storage, and 
// the key in there does not match the key hash
// TODO: return NULL ptr instead of an err value
uintptr_t hm_find_item(void * ptr, uint64_t key, size_t item_size){
    // start looking for the key in the metadata
    key_meta_group* meta = hm_bare_meta_ptr(ptr, item_size);
    if (meta == NULL){
        return UINTPTR_MAX;
    }
    uint64_t hash = C_DS_HASH_FUNC((uint8_t*)&key, sizeof(uint64_t));
    uintptr_t truncated_hash = truncate_to_cap(hash, ptr);
    // meta groups are groups of 16, so find which group this is in.
    uintptr_t group_num = truncated_hash/GROUP_SIZE;
    uint16_t group_i = truncated_hash - group_num*GROUP_SIZE;

    uint8_t lower_7 = 0x7f & meta[group_num].meta[group_i];
    // if the storage bit is set, then there is no hit here.
    // We also need to check if we should keep looking for items
    if (hmap_storage_bit_set(meta[group_num].meta[group_i]) || 
            (key != meta[group_num].keys[group_i] && lower_7 == 0)){
        // no item found
        return UINTPTR_MAX;
    }

    do{
        if (key == meta[group_num].keys[group_i]){
            return group_num*GROUP_SIZE + group_i;
        }

        // go through the linked list until we get to the end.
        uintptr_t j_dist = jump_distance(meta[group_num].meta[group_i]);
        uintptr_t new_i = truncate_to_cap(j_dist + group_num*GROUP_SIZE + group_i, ptr);
        group_num = new_i/GROUP_SIZE;
        group_i = new_i - group_num*GROUP_SIZE;
    }while ((meta[group_num].meta[group_i] & 0x7f) != 0);

    // we did not find it if we get here:
    return UINTPTR_MAX;
}

// return the index of an empty slot
// assume finding a 0 key means the slot is open.
// if the storage bit is set then you can insert into that slot
// TODO add size checks
uintptr_t hm_find_empty_slot(void * ptr, uint64_t key, size_t item_size){
    // start looking for the key in the metadata
    key_meta_group* meta = hm_bare_meta_ptr(ptr, item_size);
    if (meta == NULL){
        return UINTPTR_MAX;
    }
    uint64_t hash = C_DS_HASH_FUNC((uint8_t*)&key, sizeof(uint64_t));
    uintptr_t truncated_hash = hash & (hm_cap(ptr) - 1);
    // meta groups are groups of 16, so find which group this is in.
    uintptr_t group_num = truncated_hash/GROUP_SIZE;
    uint16_t group_i = truncated_hash - group_num*GROUP_SIZE;

    uint8_t lower_7 =  0;
    do{
        lower_7 = 0x7f & meta[group_num].meta[group_i];

        // found an empty slot
        if (hmap_storage_bit_set(meta[group_num].meta[group_i]) || 
                (meta[group_num].keys[group_i] == 0 && lower_7 == 0)){
            return group_num*GROUP_SIZE + group_i;
        }
        // go through the linked list until we get to the end.
        if (lower_7 > 0){
            // jump to the next item
            uintptr_t j_dist = jump_distance(meta[group_num].meta[group_i]);
            uintptr_t new_i = truncate_to_cap(j_dist + group_num*GROUP_SIZE + group_i, ptr);
            group_num = new_i/GROUP_SIZE;
            group_i = new_i - group_num*GROUP_SIZE;
        }
        else {
            // we are at the end of the linked list, so try out jump distances to see if we can find a fit
            for (uint8_t i = 1; i <=0x7f; ++i){
                uintptr_t j_dist = jump_distance(i);
                uintptr_t new_i = truncate_to_cap(j_dist + group_num*GROUP_SIZE + group_i, ptr);
                uintptr_t tmp_grp = new_i/GROUP_SIZE;
                uintptr_t tmp_i = new_i - tmp_grp*GROUP_SIZE;

                // see if the slot is empty
                if (hmap_storage_bit_set(meta[tmp_grp].meta[tmp_i]) ||
                        (meta[tmp_grp].keys[tmp_i] == 0 && (meta[tmp_grp].meta[tmp_i] & 0x7f) == 0)){
                    return new_i;
                }

            }
            // TODO: reallocate at this point.
            return UINTPTR_MAX;
        }

    }while (lower_7 > 0);

    // we did not find it if we get here:
    return UINTPTR_MAX;
}

void *hm_bare_set(void * ptr, uint64_t key, void * val_ptr, uintptr_t val_size);

#define hm_set(ptr, key, val) ptr = hm_bare_set(ptr, key, &val, sizeof(val))

void* hm_bare_realloc(void * ptr,uintptr_t item_count, uintptr_t item_size){

    c_ds_info *base_ptr = NULL;
    if (ptr != NULL){
        base_ptr = hm_info_ptr(ptr);
    }

    uintptr_t new_cap = next_pow2(item_count);

    uintptr_t num_key_blocks = (new_cap + (GROUP_SIZE-1))/GROUP_SIZE;
    uintptr_t new_size = new_cap*item_size + num_key_blocks*sizeof(key_meta_group) + sizeof(c_ds_info);

    c_ds_info* new_ptr = C_DS_REALLOC(NULL, new_size);
    if (new_ptr == NULL){
        hm_set_err(ptr, ds_alloc_fail);
        return ptr;
    } else {
        new_ptr->cap = new_cap;
        new_ptr->outside_mem = false;
        new_ptr->err = ds_success;
        new_ptr->len = 0;
        // copy over necessary parts of the old table if there is one.
        if (base_ptr == NULL || base_ptr->cap == 0){
            // previous table was empty, don't coyp or re-insert anything
            new_ptr->len = 0;
            ++new_ptr;
            memset(new_ptr, 0, new_size - sizeof(c_ds_info));
            return new_ptr;
        } else {
            ++new_ptr;
            memset(new_ptr, 0, new_size - sizeof(c_ds_info));

            // go through the meta table and re-hash everything to insert
            // it into the new table.
            key_meta_group* old_meta = hm_bare_meta_ptr(ptr, item_size);
            // TODO: run insert in a loop while iterating over the old
            // table
            for (uintptr_t key_i = 0; base_ptr->len > 0 && key_i < hm_cap(new_ptr); ++key_i){
                // if you find a 0 key, then skip it.
                uintptr_t group_num = key_i/GROUP_SIZE;
                uint16_t group_i = key_i - group_num*GROUP_SIZE;
                uint64_t key = old_meta[group_num].keys[group_i];
                if (key != 0){
                    uint8_t * val_ptr = (uint8_t*)ptr + item_size*key_i;
                    hm_bare_set(new_ptr, key, val_ptr, item_size); 
                    if (hm_is_err_set(new_ptr)){
                        // return the old table
                        void * _free_me = C_DS_REALLOC(new_ptr, 0);
                        (void)_free_me;
                        base_ptr->err = ds_not_found;
                        return ptr;
                    }
                    --base_ptr->len;
                }
            }
            // free old memory
            void * _ignore_this = C_DS_REALLOC(base_ptr, 0);
            (void)_ignore_this;
            return new_ptr;
        }
    }
}

#define hm_realloc(ptr, new_cap) ptr = hm_bare_realloc(ptr, new_cap, sizeof(*ptr))

void *hm_bare_set(void * ptr, uint64_t key, void * val_ptr, uintptr_t val_size){
    uintptr_t __found_location_dex = hm_find_empty_slot(ptr, key, val_size);
    uint8_t* byte_ptr = (uint8_t*)ptr;
    if (__found_location_dex != UINTPTR_MAX){
        uint16_t __c_ds_group_num = __found_location_dex/GROUP_SIZE;
        uint16_t __c_ds_group_i = __found_location_dex - __c_ds_group_num*GROUP_SIZE;
        key_meta_group * meta_ptr = hm_bare_meta_ptr(ptr, val_size);
        meta_ptr[__c_ds_group_num].keys[__c_ds_group_i] = key;
        memcpy(&byte_ptr[val_size*__found_location_dex], val_ptr, val_size);
        hm_info_ptr(ptr)->len++;
    } else {
        // try to reallocate
        ptr = hm_bare_realloc(ptr,hm_cap(ptr) + 1, val_size);
        if (hm_is_err_set(ptr)){
            return ptr;
        }

        ptr = hm_bare_set(ptr, key, val_ptr, val_size);
        if (hm_is_err_set(ptr)){
            return ptr;
        }
        hm_set_err(ptr, ds_not_found);
    }
    return ptr;
}

