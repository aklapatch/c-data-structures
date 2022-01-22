#pragma once
#include "ant_hash.h"
#include "dynarr.h"
#include "bit_setting.h"

#define KEY_TS (UINTPTR_MAX)

#define PROBE_STEP (GROUP_SIZE)

// keep as a pow2, a for loop uses this -1 as a mask
#define GROUP_SIZE (8)

#define GROUP_MASK (GROUP_SIZE - 1)

// with the hmap_bench this hits diminishing returns around 20-40
#define PROBE_TRIES (9)

#define one_i_to_val_is(main_i, bucket_i, key_i) bucket_i = (main_i)/8; key_i = main_i - (bucket_i*8)
#define val_is_to_one_i(main_i, bucket_i, key_i) (main_i) = bucket_i*8 + key_i

#define one_i_to_bucket_is(main_i, bucket_i, key_i) bucket_i = (main_i)/GROUP_SIZE; key_i = main_i - (bucket_i*GROUP_SIZE)
#define bucket_is_to_one_i(main_i, bucket_i, key_i) (main_i) = bucket_i*GROUP_SIZE + key_i

#define RND_TO_GRP_NUM(x) ((x + (GROUP_SIZE-1))/GROUP_SIZE)

// define the dict as it's own thing, separate from the hmap, it has different needs.

typedef struct {
    uintptr_t keys[GROUP_SIZE];
    uint32_t val_i[GROUP_SIZE]; 
    uint32_t key_i[GROUP_SIZE];
} hash_bucket;

// hash function prototype
typedef uintptr_t (*hash_fn_t)(uintptr_t);

typedef struct hm_info{
    hash_fn_t hash_func;
    // holds the metadata for the hash table.
    hash_bucket* buckets;
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

hash_bucket* hm_bucket_ptr(void * ptr){
    return (ptr == NULL) ? NULL : hm_info_ptr(ptr)->buckets;
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
    uintptr_t target_thresh = (hm_cap(ptr)*12)/16;
    return hm_num(ptr) >= target_thresh;
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
    free_helper(hm_bucket_ptr(ptr), realloc_fn);
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

    uintptr_t key_ret_i = UINTPTR_MAX;

    hash_fn_t hash_fn = hm_hash_func(ptr);

    hash_bucket* buckets = hm_bucket_ptr(ptr);
    if (dex_slot_out != NULL) { *dex_slot_out = UINTPTR_MAX; }

    uint8_t i = 0;
    uintptr_t hash = hash_fn(key), truncated_hash; // save the hashes for the value search loop
    uintptr_t step = PROBE_STEP;
    uintptr_t cap_mask = hm_cap(ptr) - 1;
    for (; i < PROBE_TRIES; ++i){
        // use hash first time, quadratic probe every other time
        truncated_hash = hash & cap_mask;

        uintptr_t bucket_i; uint8_t key_i;
        one_i_to_bucket_is(truncated_hash , bucket_i, key_i);
        hash_bucket *bucket = buckets + bucket_i;
        uintptr_t *keys = bucket->keys;
        uint32_t *val_i = bucket->val_i;

        uint8_t j = key_i, times = GROUP_SIZE;
        for (; times > 0; --times, j = (j + 1) & GROUP_MASK){
            if (find_empty){
                // look through the slot meta to find an empty slot.
                // We don't need to look through every slot in the bucket since
                // the slot_meta array is not a bucket
                // search the bucket and see if we can insert
                // look for the key
                if (keys[j] == KEY_TS){
                    bucket_is_to_one_i(key_ret_i, bucket_i, j);
                    goto val_search;
                }
            } else {
                // search the bucket and see if we can insert
                // look for the key
                if (keys[j] == key && key != KEY_TS){
                    if (dex_slot_out != NULL) { *dex_slot_out = val_i[j]; }
                    bucket_is_to_one_i(key_ret_i, bucket_i, j);
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
    } else if (dex_slot_out != NULL){
        // return the dex slot if we're searching for a key
        uintptr_t key_bucket; uint8_t key_i;
        one_i_to_bucket_is(key_ret_i, key_bucket, key_i);
        *dex_slot_out = buckets[key_bucket].val_i[key_i];
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

    uintptr_t bucket_i; uint8_t key_i;
    one_i_to_bucket_is(key_dex, bucket_i, key_i);
    hash_bucket *buckets = hm_bucket_ptr(ptr);
    buckets[bucket_i].val_i[key_i] = dex;
    buckets[bucket_i].keys[key_i] = key;
    //
    // set the key location for this value
    uint8_t val_i;
    one_i_to_val_is(dex, bucket_i, val_i);
    buckets[bucket_i].key_i[val_i] = key_dex;

    return 0;
}

// handle both the init and growing case, but not shrinking yet.
void *hm_raw_grow(void * ptr, realloc_fn_t realloc_fn, hash_fn_t hash_func, uintptr_t item_count, uintptr_t item_size){

    uint8_t greater_size = (GROUP_SIZE > 8) ? GROUP_SIZE : 8;
    item_count = (item_count < 2*greater_size) ? 2*greater_size : item_count;

    // shrinking is not implemented yet
    if (item_count < hm_cap(ptr)){
        return ptr;
    }

    // should be null safe, base_ptr will be null if ptr is null
    hm_info *base_ptr = hm_info_ptr(ptr);

    uintptr_t new_cap = next_pow2(item_count);

    uintptr_t old_num_buckets = hm_cap(ptr)/GROUP_SIZE;
    uintptr_t num_buckets = (new_cap + (GROUP_SIZE-1))/GROUP_SIZE;
    uintptr_t bucket_size = num_buckets*sizeof(hash_bucket);
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

    // old_bucket_ptr is not necessary if allocating from scratch
    hash_bucket *old_bucket_ptr = inf_ptr->buckets;
    hash_bucket *bucket_ptr = realloc_fn(NULL, bucket_size);
    if (bucket_ptr == NULL){
        ++inf_ptr;
        hm_set_err(inf_ptr, ds_alloc_fail);
        return inf_ptr;
    }

    // associate the key pointer with the new structure
    inf_ptr->buckets = bucket_ptr;
    inf_ptr->cap = new_cap;

    // set the new keys
    for (uintptr_t i = 0; i < num_buckets; ++i){
        for (uint8_t j = 0; j < GROUP_SIZE; ++j){
            inf_ptr->buckets[i].keys[j] = KEY_TS;
        }
    }
    ++inf_ptr;

    // we should be done if we are just allocating
    if (base_ptr == NULL){
        return inf_ptr;
    }

    uintptr_t num_items = hm_num(inf_ptr);
    // search the old key structure for keys
    for (uintptr_t bucket_i = 0; bucket_i < old_num_buckets; ++bucket_i){
        for (uint8_t i = 0; i < GROUP_SIZE; ++i){
            if (old_bucket_ptr[bucket_i].keys[i] != KEY_TS){
                uintptr_t key = old_bucket_ptr[bucket_i].keys[i];
                uint32_t dex = old_bucket_ptr[bucket_i].val_i[i];
                uintptr_t ret = insert_key_and_dex(
                        inf_ptr, 
                        key, 
                        dex);
                if (ret == UINTPTR_MAX){
                    hm_info_ptr(inf_ptr)->buckets = old_bucket_ptr;
                    // free the old memory
                    free_helper(bucket_ptr, realloc_fn);
                    goto done;
                } 
                if (num_items == 0){
                    goto free_old_bucket;
                }
            }
        }
    }

    // success, free old buckets
free_old_bucket:
    free_helper(old_bucket_ptr, realloc_fn);

done:
    hm_set_err(inf_ptr, ds_success);
    return inf_ptr;
}

#define hm_grow(ptr, new_cap) ptr = hm_raw_grow(ptr, hm_realloc_fn(ptr), hm_hash_func(ptr), new_cap, sizeof(*ptr))
#define hm_init(ptr, num_items, realloc_fn, hash_func) ptr = hm_raw_grow(NULL, realloc_fn, hash_func, num_items, sizeof(*ptr))

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

    uintptr_t bucket_i; uint8_t key_i;
    one_i_to_bucket_is(key_dex_out, bucket_i, key_i);

    hash_bucket *buckets = hm_bucket_ptr(ptr);
    // only increment the num if we are not replacing a key
    if (buckets[bucket_i].keys[key_i] == KEY_TS){
        hm_info_ptr(ptr)->num++;
        // set that the slot is taken
    }
    buckets[bucket_i].keys[key_i] = key;
    buckets[bucket_i].val_i[key_i] = val_dex;

    // set the key location for this value
    uint8_t val_i;
    one_i_to_val_is(val_dex, bucket_i, val_i);
    buckets[bucket_i].key_i[val_i] = key_dex_out;

    return val_dex;
}

// sets ptr->tmp_val_i to something useful on success
void *hm_try_insert(void *ptr, uintptr_t key, size_t item_size){
    if (ptr == NULL){
#ifdef _STDLIB_H
        ptr = hm_raw_grow(ptr, realloc, ant_hash, 16, item_size);
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
        ptr = hm_raw_grow(
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

// TODO: STB just puts the newest element at the end of the array and memmoves 
// an old element over the removed one. Implement that so we can get rid of val_meta
// We don't need to really worry about it right now since queries are about as
// fast as STB's
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

    hash_bucket * buckets = hm_bucket_ptr(ptr);

    hm_info_ptr(ptr)->num--;

    // the del macro should move the end val_i to the spot we just deleted
    uintptr_t bucket_i; uint8_t key_i;
    one_i_to_bucket_is(key_dex, bucket_i, key_i);
    buckets[bucket_i].keys[key_i] = KEY_TS;

    // only move the val slot if the val slot is not the last slot
    if (val_dex != hm_num(ptr)){
        // get the key index of the last val slot, and then move it to 
        // the slot that the last val will get moved to
        uintptr_t val_i; uint8_t val_j;
        one_i_to_bucket_is(val_dex, val_i, val_j);

        uint32_t end_i; uint8_t end_j;
        one_i_to_bucket_is(hm_num(ptr), end_i, end_j);

        // grab the key_i for the last key and set the opened slot 
        // to point to the end_key
        uint32_t end_key_i = buckets[end_i].key_i[end_j];
        buckets[val_i].key_i[val_j] = end_key_i;

        // set the val_i of the key for the end val to point to the new slot.
        one_i_to_bucket_is(end_key_i, bucket_i, key_i);
        buckets[bucket_i].val_i[key_i] = val_dex;
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
        }\
    } while(0)
