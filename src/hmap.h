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


// constants for the metadata
// Empty slot
#define META_EMPTY ((uint8_t)0xFF)

// Bits set for a non-direct hit
#define STORAGE_BIT_MASK ((uint8_t)0x80)

// Mask to get the list offset
#define LIST_OFF_MASK ((uint8_t)0x7f)

#define DEX_TS ((uintptr_t)UINTPTR_MAX)

#define GROUP_SIZE (16)
typedef struct {
    //MSB set key slot is storage
    uint8_t key_meta[GROUP_SIZE];
    uint64_t keys[GROUP_SIZE];
    // UINTPTR_MAX means key slot is open
    uintptr_t indexs[GROUP_SIZE];

    // This meta represents if the value slot is open or not.
    // 0xFF, value slot is free
    // 0, value slot is taken (may use it to figure out where next open slot is)
    uint8_t value_meta[GROUP_SIZE];
} key_meta_group;

typedef struct hm_info{
    uintptr_t num,cap;
    key_meta_group* meta;
    ds_error_e err;
    bool outside_mem, hash_table;
} hm_info;
//
// For the memory layout of the hash table:
// [info struct][data (ptr points here[0])][keys]
// keys are uint64_t for keys
hm_info * hm_info_ptr(void * ptr){
    return (ptr == NULL) ? NULL : (key_meta_group*)ptr - 1;
}

uintptr_t hm_cap(void * ptr){
    hm_info_ptr* tmmp = hm_info_ptr(ptr);
    return (tmmp == NULL) ? 0 : hm_info_ptr(ptr)->cap;
}

uintptr_t hm_num(void * ptr){
    hm_info_ptr* tmmp = hm_info_ptr(ptr);
    return (tmmp == NULL) ? 0 : hm_info_ptr(ptr)->num;
}

void hm_set_err(void * ptr, ds_error_e err){
    if (ptr != NULL){
        hm_info_ptr(ptr)->err = err;
    }
}

ds_error_e hm_err(void * ptr){
    if (ptr == NULL){
        return ds_null_ptr;
    }
    return hm_info_ptr(ptr)->err;
}

bool hm_is_err_set(void * ptr){
    return hm_err(ptr) != ds_success;
}

char * hm_err_str(void *ptr){
    return ds_get_err_str(hm_err(ptr));
}

// API:
// - find (key) -> index
// - set (key, val) -> err
// - get (key) -> val
// - set_cap

bool hm_slot_empty(uint8_t num){
    return num == META_EMPTY;
}

bool hm_slot_storage(uint8_t num){
    return num != META_EMPTY && num >= STORAGE_BIT_MASK;
}

uint8_t hm_list_off(uint8_t num){
    return num & LIST_OFF_MASK;
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

uintptr_t truncate_to_cap(void* ptr, uintptr_t num){
    return num & (hm_cap(ptr) - 1);
}


uintptr_t jump_distance(uint8_t i){
    // truncate to lower 7 bits
    i = hm_list_off(i);
    uintptr_t tri_input = (i - 10)*(i > 15);
    uintptr_t i_minus = (i - 81)*(i >= 82);
    tri_input += (i_minus + i_minus*i_minus + i_minus*i_minus*i_minus);
    uintptr_t tri_out = (tri_input*(tri_input+1)) >> 1;
    return i*(i < 16) + tri_out;
}

void * hm_base_ptr(void* ptr){
    return (ptr == NULL) ? NULL : (void*)((key_meta_group*)ptr - hm_cap(ptr));
}

key_meta_group* hm_bare_meta_ptr(void * ptr){
    return (ptr == NULL) ? NULL : (key_meta_group*)hm_info_ptr(ptr) - 1;
}

#define hm_meta_ptr(ptr) hm_bare_meta_ptr(ptr)

void *hm_bare_set(void * ptr, uint64_t key, void * val_ptr, uintptr_t val_size);

#define RND_TO_GRP_NUM(x) ((x + (GROUP_SIZE-1))/GROUP_SIZE)
#define hm_set(ptr, key, val) ptr = hm_bare_set(ptr, key, &val, sizeof(val))

// The meta table in the info struct needs to be allocated too
void* hm_bare_realloc(void * ptr,uintptr_t item_count, uintptr_t item_size){

    c_ds_info *base_ptr = NULL;
    if (ptr != NULL){
        base_ptr = hm_info_ptr(ptr);
    }

    uintptr_t new_cap = next_pow2(item_count);

    uintptr_t num_key_blocks = (new_cap + (GROUP_SIZE-1))/GROUP_SIZE;
    uintptr_t new_size = new_cap*item_size + num_key_blocks*sizeof(key_meta_group) + sizeof(c_ds_info);

    key_meta_group* new_ptr = C_DS_REALLOC(NULL, new_size);
    if (new_ptr == NULL){
        hm_set_err(ptr, ds_alloc_fail);
        return ptr;
    } else {
        // set all the metadata to empty
        for (uintptr_t i = 0; i < num_key_blocks; ++i){
            memset(new_ptr[i].meta, META_EMPTY, sizeof(new_ptr[i].meta));
        }
        // get the past the key meta groups.
        c_ds_info * new_inf_ptr = (c_ds_info*)(new_ptr + num_key_blocks);
        new_inf_ptr->cap = new_cap;
        new_inf_ptr->outside_mem = false;
        new_inf_ptr->err = ds_success;
        new_inf_ptr->len = 0;
        ++new_inf_ptr;

        // copy over necessary parts of the old table if there is one.
        if (base_ptr == NULL || base_ptr->cap == 0){
            // previous table was empty, don't coyp or re-insert anything
            return new_inf_ptr;
        } else {
            // go through the meta table and re-hash everything to insert
            // it into the new table.
            key_meta_group* old_meta = (key_meta_group*)hm_info_ptr(ptr) - 1;
            for (uintptr_t i = 0; base_ptr->len > 0 && i < hm_cap(new_inf_ptr); ++i){
                    key_meta_group *tmp_ptr = old_meta - (i/GROUP_SIZE);
                    uint16_t grp_i =  i - (i/GROUP_SIZE)*GROUP_SIZE;
                    if (tmp_ptr->meta[grp_i] != META_EMPTY){
                        uint8_t * val_ptr = (uint8_t*)ptr + item_size*i;
                        hm_bare_set(new_inf_ptr, tmp_ptr->keys[grp_i], val_ptr, item_size); 
                        if (hm_is_err_set(new_inf_ptr)){
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
            C_DS_FREE(hm_base_ptr(ptr));
            return new_inf_ptr;
        }
    }
}

#define hm_realloc(ptr, new_cap) ptr = hm_bare_realloc(ptr, new_cap, sizeof(*ptr))

void hm_bare_set_member(void * ptr, uintptr_t set_i){

}

#define hm_set(ptr, key, val) hm_bare_set(ptr, key, &val, sizeof(val))

// returns UINTPTR_MAX on failure
uintptr_t hm_raw_find_open_slot(void* ptr, uint64_t key){

}

void hm_dex_to_dexs(uintptr_t main_i, uintptr_t * group_num, uint16_t* inner_i){
    *group_num = main_i/GROUP_SIZE;
    *inner_i = main_i - *group_num;
}

key_meta_group* hm_ptr_to_group(void* ptr, uintptr_t group_num){
    return (hm_bare_meta_ptr(ptr) == NULL) ? NULL : hm_bare_meta_ptr(ptr) - group_num;
}
#if 0
#define hm_test_set(ptr, k, v)\
    do{\
        uintptr_t _found_dex_ = find_empty_or_storage_dex(ptr, k);
        if (_found_dex != UINTPTR_MAX){


        }

    }while(0)
#endif

void hm_copy_val_in(void* ptr, uintptr_t dex, void* val_in, size_t val_size){
    uint8_t *byte_ptr = (uint8_t*)ptr + val_size*dex;
    memcpy(byte_ptr, val_in, val_size);
}
void hm_copy_val_out(void* ptr, uintptr_t dex, void* val_out, size_t val_size){
    uint8_t *byte_ptr = (uint8_t*)ptr + val_size*dex;
    memcpy(val_out, byte_ptr, val_size);
}

// This table uses jump distances to put other items into the array of keys and items
// Theh top bit is set if a matching hash is there, the rest of the 7 bits are the jump distance
// if the jump distance is 0 and the  key does not match, then there are no more items matching that
// hash, if the top bit of the byte array is set, then the slot is being used for storage, and 
// the key in there does not match the key hash
void *hm_bare_set(void * ptr, uint64_t key, void * val_ptr, uintptr_t val_size){
    // start looking for the key in the metadata
    key_meta_group* meta = hm_bare_meta_ptr(ptr);
    if (meta == NULL){
        return ptr;
    }

    if (hm_cap(ptr) == hm_num(ptr)){
        ptr = hm_bare_realloc(ptr, hm_cap(ptr) + 1, val_size);
        if (hm_is_err_set(ptr)){
            return ptr;
        }
    }

    uint64_t hash = C_DS_HASH_FUNC((uint8_t*)&key, sizeof(uint64_t));
    uintptr_t truncated_hash = truncate_to_cap(ptr, hash);
    // meta groups are groups of 16, so find which group this is in.
    uintptr_t group_num; uint16_t group_i;
    hm_dex_to_dexs(truncated_hash, &group_num, &group_i);

    key_meta_group* search_meta = meta - group_num;

    uint8_t list_off = hm_list_off(search_meta->meta[group_i]);
    do{
        // found an empty slot
        if (hm_slot_empty(search_meta->meta[group_i]) || search_meta->keys[group_i] == key){
            search_meta->keys[group_i] = key;
            search_meta->meta[group_i] = 0;
            hm_copy_val_in(ptr, group_num*GROUP_SIZE + group_i, val_ptr, val_size);
            hm_info_ptr(ptr)->len++;
            hm_set_err(ptr, ds_success);

            return ptr;
        } else if (hm_slot_storage(search_meta->meta[group_i])){

            // hard, I need to move a key around to get it to 
            // fit in with it's original hit and the end of
            // wherever the linked list reaches
            // the key I am replacing
            hm_set_err(ptr, ds_unimp);
            return ptr;
        }

        // go through the linked list until we get to the end.
        if (list_off > 0){
            // jump to the next item
            uintptr_t j_dist = jump_distance(list_off);
            uintptr_t new_i = truncate_to_cap(ptr, j_dist + group_num*GROUP_SIZE + group_i);
            hm_dex_to_dexs(new_i, &group_num, &group_i);
        }
        list_off = hm_list_off(search_meta->meta[group_i]);
    } while (list_off > 0);

    // we are at the end of the linked list, so try out jump distances to see if we can find a fit
    for (uint8_t i = 1; i <= LIST_OFF_MASK; ++i){
        uintptr_t j_dist = jump_distance(i);
        uintptr_t new_i = truncate_to_cap(ptr, j_dist + group_num*GROUP_SIZE + group_i);
        uintptr_t tmp_grp; uint16_t tmp_i;
        hm_dex_to_dexs(new_i, &tmp_grp, &tmp_i);

        search_meta = hm_ptr_to_group(ptr, tmp_grp);

        // see if the slot is empty
        if (hm_slot_empty(search_meta->meta[tmp_i])){
            uint8_t *byte_ptr = (uint8_t*)ptr + val_size*new_i;
            search_meta->keys[tmp_i] = key;
            search_meta->meta[tmp_i] = STORAGE_BIT_MASK;
            hm_copy_val_in(ptr, new_i, val_ptr, val_size);
            hm_info_ptr(ptr)->len++;

            // set the offset for the previous item we hit
            search_meta = meta - group_num;
            uint8_t old_val = search_meta->meta[group_i];
            // mask off the lower bits and set them to something else
            search_meta->meta[group_i] = (old_val & STORAGE_BIT_MASK) | i;

            hm_set_err(ptr, ds_success);
            return ptr;
        }
    }

    hm_set_err(ptr, ds_not_found);
    return ptr;
}

#define hm_get(ptr, key, val_ptr) hm_bare_get(ptr, key, val_ptr, sizeof(*(ptr)))

void hm_bare_get(void * ptr, uint64_t key, void * out_ptr, size_t val_size){
    uint64_t hash = C_DS_HASH_FUNC((uint8_t*)&key, sizeof(uint64_t));
    uintptr_t truncated_hash = truncate_to_cap(ptr, hash);
    // meta groups are groups of 16, so find which group this is in.
    uintptr_t group_num = truncated_hash/GROUP_SIZE;
    uint16_t group_i = truncated_hash - group_num*GROUP_SIZE;

    key_meta_group* search_meta = hm_meta_ptr(ptr) - group_num;

    do {
        if (!hm_slot_empty(search_meta->meta[group_i]) && search_meta->keys[group_i] == key){
            uint8_t * byte_ptr = (uint8_t*)ptr + val_size*(group_num*GROUP_SIZE + group_i);
            memcpy(out_ptr, byte_ptr, val_size);
            hm_set_err(ptr, ds_success);
            return;
        }

        // iterate through the linked list of items if there is one.
        if (hm_list_off(search_meta->meta[group_i]) == 0){
            hm_set_err(ptr, ds_not_found);
            return;
        }

        uintptr_t j_dist = jump_distance(search_meta->meta[group_i]);
        uintptr_t new_i = truncate_to_cap(ptr, j_dist + group_num*GROUP_SIZE + group_i);
        group_num = new_i/GROUP_SIZE;
        group_i = new_i - group_num*GROUP_SIZE;
        search_meta = hm_meta_ptr(ptr) - group_num;
    } while (true);
    
    hm_set_err(ptr, ds_not_found);
}
