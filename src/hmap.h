#pragma once
#include "ant_hash.h"
#include "dynarr.h"
#include "bit_setting.h"
#include "misc.h"

#define KEY_TS (UINTPTR_MAX)

#define PROBE_STEP (GROUP_SIZE)

// keep as a pow2, a for loop uses this -1 as a mask
#define GROUP_SIZE (4)

#define GROUP_MASK (GROUP_SIZE - 1)

// with the hmap_bench this hits diminishing returns around 20-40
#define PROBE_TRIES (9)

#define one_i_to_val_is(main_i, bucket_i, key_i) bucket_i = (main_i)/8; key_i = main_i - (bucket_i*8)
#define val_is_to_one_i(main_i, bucket_i, key_i) (main_i) = bucket_i*8 + key_i

#define one_i_to_bucket_is(main_i, bucket_i, key_i) bucket_i = (main_i)/GROUP_SIZE; key_i = main_i - (bucket_i*GROUP_SIZE)
#define bucket_is_to_one_i(main_i, bucket_i, key_i) (main_i) = bucket_i*GROUP_SIZE + key_i

#define RND_TO_GRP_NUM(x) ((x + (GROUP_SIZE-1))/GROUP_SIZE)

// define the dict as it's own thing, separate from the hmap, it has different needs.

// hash function prototype
typedef uintptr_t (*hash_fn_t)(uintptr_t);

typedef struct hm_info{
    hash_fn_t hash_func;
    // holds the metadata for the hash table.
    uintptr_t *keys;
    uint32_t *val_i,*key_i;
    realloc_fn_t realloc_fn;
    // tmp_val_i is used to set the value array in the macros
    uintptr_t cap,num, tmp_val_i;
    uint8_t err,outside_mem;
} hm_info;

hm_info * hm_info_ptr(void * ptr){
    return (ptr == NULL) ? NULL : (hm_info*)ptr - 1;
}

realloc_fn_t hm_realloc_fn(void *ptr){
    return (ptr == NULL) ? NULL : hm_info_ptr(ptr)->realloc_fn;
}

hash_fn_t hm_hash_func(void *ptr){
    return (ptr == NULL) ? NULL : hm_info_ptr(ptr)->hash_func;
}

uintptr_t hm_cap(void * ptr){
    hm_info* tmmp = hm_info_ptr(ptr);
    return (tmmp == NULL) ? 0 : tmmp->cap;
}

uintptr_t hm_num(void * ptr){
    hm_info* tmmp = hm_info_ptr(ptr);
    return (tmmp == NULL) ? 0 : tmmp->num;
}

bool should_grow(void *ptr){
    uintptr_t target_thresh = (hm_cap(ptr)*14)/16;
    return hm_num(ptr) >= target_thresh;
}

// once we get below the should_grow threshold for the next power of 2 don, then shrink
bool should_shrink(void *ptr){
    uintptr_t lower_pow2 = hm_cap(ptr)/2;
    uintptr_t threshold = (lower_pow2*14)/16;
    return hm_num(ptr) < threshold;
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

void free_helper(void *ptr, realloc_fn_t realloc_fn){
    if (ptr != NULL){
        void * ignore_ptr = realloc_fn(ptr, 0);
        (void)ignore_ptr;
    }
}
void _hm_free(void * ptr, realloc_fn_t realloc_fn){
    free_helper(hm_info_ptr(ptr)->keys, realloc_fn);
    free_helper(hm_info_ptr(ptr)->val_i, realloc_fn);
    free_helper(hm_info_ptr(ptr)->key_i, realloc_fn);
    free_helper(hm_info_ptr(ptr), realloc_fn);
}

#define hm_free(ptr) _hm_free(ptr, hm_realloc_fn(ptr)),ptr=NULL

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

// This function is used for
// - finding a key slot
// - finding a key to delete
// - finding a value index
// - probe for slots 
// - finding an empty slot
// - finding a slot with a key in it
//
// if dex_slot_out is NULL, then don't look for a dex slot
// returns key slot
static uintptr_t key_find_helper(
    void *ptr,
    uintptr_t key,
    uintptr_t *dex_slot_out,
    bool find_empty){

    // only error out regarding size constraints when looking for an empty slot
    if (!find_empty && hm_num(ptr) == hm_cap(ptr)){ return UINTPTR_MAX; }

    // bail on a bad key_ret_i
    if (key == KEY_TS){
        hm_set_err(ptr, ds_bad_param);
        return UINTPTR_MAX;
    }

    uintptr_t key_ret_i = UINTPTR_MAX;

    hash_fn_t hash_fn = hm_hash_func(ptr);

    if (dex_slot_out != NULL) { *dex_slot_out = UINTPTR_MAX; }

    uint8_t i = 0;
    uintptr_t hash = hash_fn(key), truncated_hash; // save the hashes for the value search loop
    uintptr_t step = PROBE_STEP;
    uintptr_t cap_mask = hm_cap(ptr) - 1;
    uintptr_t *keys = hm_info_ptr(ptr)->keys;
    for (; i < PROBE_TRIES; ++i){
        // use hash first time, quadratic probe every other time
        truncated_hash = hash & cap_mask;

        uintptr_t start_i = (truncated_hash/GROUP_SIZE)*GROUP_SIZE;
        uint8_t j = truncated_hash - start_i, times = GROUP_SIZE;
        for (; times > 0; --times, j = (j + 1) & GROUP_MASK){
            if (find_empty){
                // look through the slot meta to find an empty slot.
                // We don't need to look through every slot in the bucket since
                // the slot_meta array is not a bucket
                // search the bucket and see if we can insert
                // look for the key
                if (keys[start_i + j] == KEY_TS){
                    key_ret_i = start_i + j;
                    goto val_search;
                }
            } else {
                // search the bucket and see if we can insert
                // look for the key
                if (keys[start_i + j] == key){
                    key_ret_i = start_i + j;
                    if (dex_slot_out != NULL) { *dex_slot_out = hm_info_ptr(ptr)->val_i[key_ret_i]; }
                    return key_ret_i;
                }
            }
        }
        hash = truncated_hash + step;
        step += PROBE_STEP;
    }
    if (i == PROBE_TRIES || key_ret_i == UINTPTR_MAX){ return UINTPTR_MAX; }

val_search:
    ;
    // start looking through everything for a val slot
    // use the old values of bucket_i and key_i
    if (dex_slot_out != NULL && find_empty){
        // return the next slot in the value array
        // This 
        *dex_slot_out = hm_num(ptr);
    } 

    return key_ret_i;
}

// basically an insert, but we don't need to look for a dex slot
static uintptr_t insert_key_and_dex(void *ptr, uintptr_t key, uint32_t dex){

    if (hm_num(ptr) == hm_cap(ptr)){ return UINTPTR_MAX; }

    uintptr_t key_dex = key_find_helper(
            ptr,
            key,
            NULL,
            true);
    if (key_dex == UINTPTR_MAX){ return UINTPTR_MAX; }

    hm_info_ptr(ptr)->keys[key_dex] = key;
    hm_info_ptr(ptr)->val_i[key_dex] = dex;

    //
    // set the key location for this value
    hm_info_ptr(ptr)->key_i[dex] = key_dex;

    return 0;
}

// handle both the init and growing case, but not shrinking yet.
void *hm_raw_realloc(void * ptr, realloc_fn_t realloc_fn, hash_fn_t hash_func, uintptr_t item_count, uintptr_t item_size){

    uint8_t greater_size = (GROUP_SIZE > 8) ? GROUP_SIZE : 8;
    item_count = (item_count < 2*greater_size) ? 2*greater_size : item_count;

    // should be null safe, base_ptr will be null if ptr is null
    hm_info *base_ptr = hm_info_ptr(ptr);

    uintptr_t old_cap = hm_cap(ptr);
    uintptr_t new_cap = next_pow2(item_count);
    
    // bail if we have too many items to shrink
    if (hm_num(ptr) > new_cap){
        hm_set_err(ptr, ds_bad_param);
        return ptr;
    }

    uintptr_t data_size = new_cap*item_size + sizeof(hm_info);

    hm_info * inf_ptr = realloc_fn(base_ptr, data_size);
    if (inf_ptr == NULL){
        hm_set_err(ptr, ds_alloc_fail);
        // old pointer is still good, return that
        return ptr;
    }
    inf_ptr->outside_mem = false;
    inf_ptr->err = ds_success;

    if (base_ptr == NULL){
        //allocating new array
        inf_ptr->num = 0;
        inf_ptr->hash_func = hash_func;
        inf_ptr->realloc_fn = realloc_fn;
    }


    void *old_ptrs[] = { inf_ptr->keys, inf_ptr->key_i, inf_ptr->val_i };
    uint8_t sizes[] = { sizeof(inf_ptr->keys[0]), sizeof(inf_ptr->key_i[0]), sizeof(inf_ptr->val_i[0]) };
    void *new_ptrs[COUNTOF(old_ptrs)];
    // old_ptrs is not needed if allocating from stratch
    for (uint8_t i = 0; i < COUNTOF(new_ptrs); ++i){
        new_ptrs[i] = realloc_fn(NULL, new_cap*sizes[i]);
        if (new_ptrs[i] == NULL){
            // free the pointers we've already allocated
            for (int8_t j = i - 1; j >= 0; --j){
                free_helper(new_ptrs[j], realloc_fn);
            }
            ++inf_ptr;
            hm_set_err(inf_ptr, ds_alloc_fail);
            return inf_ptr;
        }
    }

    // associate the key pointer with the new structure
    inf_ptr->keys = new_ptrs[0];
    inf_ptr->key_i = new_ptrs[1];
    inf_ptr->val_i = new_ptrs[2];

    inf_ptr->cap = new_cap;

    uintptr_t *old_keys = old_ptrs[0];
    uint32_t *old_key_i = old_ptrs[1], *old_val_i = old_ptrs[2];

    // set the new keys
    for (uintptr_t i = 0; i < new_cap; ++i){
        inf_ptr->keys[i] = KEY_TS;
    }
    ++inf_ptr;

    // we should be done if we are just allocating
    if (base_ptr == NULL){
        return inf_ptr;
    }

    uintptr_t num_items = hm_num(inf_ptr);
    // search the old key structure for keys
    for (uintptr_t i = 0; i < old_cap; ++i){
        if (old_keys[i] != KEY_TS){
            uintptr_t key = old_keys[i];
            uint32_t dex = old_val_i[i];

            uintptr_t ret = insert_key_and_dex(
                    inf_ptr, 
                    key, 
                    dex);

            if (ret == UINTPTR_MAX){
                hm_info_ptr(inf_ptr)->keys = old_keys;
                hm_info_ptr(inf_ptr)->key_i = old_key_i;
                hm_info_ptr(inf_ptr)->val_i = old_val_i;

                // free the old memory
                free_helper(new_ptrs[0], realloc_fn);
                free_helper(new_ptrs[1], realloc_fn);
                free_helper(new_ptrs[2], realloc_fn);

                hm_set_err(inf_ptr, ds_fail);
                goto done;
            } 
            --num_items;
            if (num_items == 0){
                goto free_old_bucket;
            }
        }

    }

    // success, free old buckets
free_old_bucket:
    free_helper(old_ptrs[0], realloc_fn);
    free_helper(old_ptrs[1], realloc_fn);
    free_helper(old_ptrs[2], realloc_fn);

    hm_set_err(inf_ptr, ds_success);
done:
    return inf_ptr;
}

#define hm_realloc(ptr, new_cap) ptr = hm_raw_realloc(ptr, hm_realloc_fn(ptr), hm_hash_func(ptr), new_cap, sizeof(*ptr))
#define hm_init(ptr, num_items, realloc_fn, hash_func) ptr = hm_raw_realloc(NULL, realloc_fn, hash_func, num_items, sizeof(*ptr))

// returns the value index
// sets the key slot found
// dex out is the index where the key is in the the bucket array
uintptr_t hm_raw_insert_key(
        void *ptr, 
        uintptr_t key)
{
    if (hm_num(ptr) == hm_cap(ptr)){ return UINTPTR_MAX; }

    uintptr_t val_dex = UINTPTR_MAX;
    uintptr_t key_dex_out = key_find_helper(
            ptr,
            key,
            &val_dex,
            true);

    if (key_dex_out == UINTPTR_MAX){ return UINTPTR_MAX; }


    uintptr_t *keys = hm_info_ptr(ptr)->keys;
    // only increment the num if we are not replacing a key
    if (keys[key_dex_out] == KEY_TS){
        hm_info_ptr(ptr)->num++;
        // set that the slot is taken
    }
    keys[key_dex_out] = key;
    hm_info_ptr(ptr)->val_i[key_dex_out] = val_dex;

    // set the key location for this value
    hm_info_ptr(ptr)->key_i[val_dex] = key_dex_out;

    return val_dex;
}

// sets ptr->tmp_val_i to something useful on success
void *hm_try_insert(void *ptr, uintptr_t key, size_t item_size){
    if (ptr == NULL){
#ifdef _STDLIB_H
        ptr = hm_raw_realloc(ptr, realloc, ant_hash, 16, item_size);
#else
#pragma message("Please init the hmap with hmap_init() or include stdlib.h to use default initialization with realloc!")
        return NULL;
#endif
    }

    // don't allow keys of UINTPTR_MAX since that is the key tombstone value
    if (key == KEY_TS){
        hm_set_err(ptr, ds_bad_param);
        return ptr;
    }

    // init the ptr if necessary
    // try the insert, and resize if needed
    for (uint8_t grow_tries = 3; grow_tries > 0; --grow_tries){
        if (!should_grow(ptr)){
            // ptr could change since we realloced, so keep getting it.
            uintptr_t *val_ptr = &hm_info_ptr(ptr)->tmp_val_i;
            *val_ptr = hm_raw_insert_key(ptr, key);
            if (*val_ptr != UINTPTR_MAX){
                // we're done
                hm_set_err(ptr, ds_success);
                return ptr;
            }
        }
        ptr = hm_raw_realloc(
            ptr,
            hm_realloc_fn(ptr),
            hm_hash_func(ptr),
            hm_cap(ptr) + 1,
            item_size);
    }
    // didn't find a slot, if the errors are already set then let them
    // go
    if (!hm_is_err_set(ptr)){
        hm_set_err(ptr, ds_not_found);
    }
    return ptr;
}

#define hm_set(ptr, k, v)\
    do{\
        ptr = hm_try_insert(ptr, k, sizeof(*(ptr)));\
        if (hm_info_ptr(ptr)->tmp_val_i != UINTPTR_MAX){\
            ptr[hm_info_ptr(ptr)->tmp_val_i] = v;\
        }\
    }while(0)

static uintptr_t hm_find_val_i(void *ptr, uintptr_t key){

    uintptr_t val_i = UINTPTR_MAX;
    uintptr_t key_dex = key_find_helper(
            ptr,
            key,
            &val_i,
            false);

    if (key_dex == UINTPTR_MAX){ 
        hm_set_err(ptr, ds_not_found);
        return UINTPTR_MAX; 
    }

    return val_i;
}

#define hm_get(ptr, key, val_to_set)\
    do {\
        hm_info_ptr(ptr)->tmp_val_i = hm_find_val_i(ptr, key);\
        if (hm_info_ptr(ptr)->tmp_val_i != UINTPTR_MAX){\
            val_to_set = ptr[hm_info_ptr(ptr)->tmp_val_i];\
        }\
    } while(0)

uintptr_t _hm_del(void *ptr, uintptr_t key){

    uintptr_t val_dex, key_dex = key_find_helper(
            ptr,
            key,
            &val_dex,
            false);

    if (key_dex == UINTPTR_MAX){
        hm_set_err(ptr, ds_not_found);
        return UINTPTR_MAX;
    }

    hm_info_ptr(ptr)->num--;

    // the del macro should move the end val_i to the spot we just deleted
    uintptr_t *keys = hm_info_ptr(ptr)->keys;
    keys[key_dex] = KEY_TS;

    // only move the val slot if the val slot is not the last slot
    if (val_dex != hm_num(ptr)){
        uint32_t *key_i = hm_info_ptr(ptr)->key_i;
        // get the key index of the last val slot, and then move it to 
        // the slot that the last val will get moved to
        
        // grab the key_i for the last key and set the opened slot 
        // to point to the end_key
        uint32_t end_key_i = key_i[hm_num(ptr)];
        key_i[val_dex] = end_key_i;

        hm_info_ptr(ptr)->val_i[end_key_i] = val_dex;
    }

    hm_set_err(ptr, ds_success);
    return val_dex;
}

// move the end value to the slot that we freed if necessary
// hm_num() has already been decremented, so we just need a < and not a < num - 1
#define hm_del(ptr, key)\
    do {\
        hm_info_ptr(ptr)->tmp_val_i = _hm_del(ptr, key);\
        if (hm_info_ptr(ptr)->tmp_val_i != UINTPTR_MAX && \
            hm_info_ptr(ptr)->tmp_val_i < hm_num(ptr)){\
            ptr[hm_info_ptr(ptr)->tmp_val_i] = ptr[hm_num(ptr)];\
        } else if (hm_info_ptr(ptr)->tmp_val_i != UINTPTR_MAX && should_shrink(ptr)){\
            ptr = hm_realloc(ptr, hm_cap(ptr)/2 - 1);\
        }\
    } while(0)
