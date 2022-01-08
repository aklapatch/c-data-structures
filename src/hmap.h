#pragma once
#include "dynarr.h"
#include "bit_setting.h"

// tombstone (empty) marker
#define DEX_TS ((uintptr_t)UINT32_MAX)

// keep as a pow2, a for loop uses this -1 as a mask
// This needs to stay 8 to correspond to 8 bits per block
#define GROUP_SIZE (8)

// with the hmap_bench this hits diminishing returns around 20-40
#define PROBE_TRIES (5)

#define one_i_to_val_is(main_i, bucket_i, key_i) bucket_i = (main_i)/8; key_i = main_i - (bucket_i*8)
#define val_is_to_one_i(main_i, bucket_i, key_i) (main_i) = bucket_i*8 + key_i

#define one_i_to_bucket_is(main_i, bucket_i, key_i) bucket_i = (main_i)/GROUP_SIZE; key_i = main_i - (bucket_i*GROUP_SIZE)
#define bucket_is_to_one_i(main_i, bucket_i, key_i) (main_i) = bucket_i*GROUP_SIZE + key_i

#define RND_TO_GRP_NUM(x) ((x + (GROUP_SIZE-1))/GROUP_SIZE)

// define the dict as it's own thing, separate from the hmap, it has different needs.

typedef struct {
    uintptr_t keys[GROUP_SIZE];
    uint32_t indices[GROUP_SIZE]; 
} hash_bucket;

// hash function prototype
typedef uintptr_t (*hash_fn_t)(void *, size_t);

typedef struct hm_info{
    hash_fn_t hash_func;
    realloc_fn_t realloc_fn;
    // holds the metadata for the hash table.
    hash_bucket* buckets;
    uint8_t *val_metas;
    // tmp_val_i is used to set the value array in the macros
    uintptr_t cap,num, tmp_val_i;
    uint8_t err,outside_mem;
} hm_info;

hm_info * hm_info_ptr(void * ptr){
    return (ptr == NULL) ? NULL : (hm_info*)ptr - 1;
}

uint8_t *hm_val_meta_ptr(void * ptr){
    return (ptr == NULL) ? NULL : hm_info_ptr(ptr)->val_metas;
}


hash_fn_t hm_hash_func(void *ptr){
    return (ptr == NULL) ? NULL : hm_info_ptr(ptr)->hash_func;
}

realloc_fn_t hm_realloc_fn(void *ptr){
    return (ptr == NULL) ? NULL : hm_info_ptr(ptr)->realloc_fn;
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

// grow around 40% full
bool should_grow(void *ptr){
    uintptr_t target_thresh = (hm_cap(ptr)*2)/5;
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

void _hm_free(void * ptr){
    if (ptr != NULL){
        realloc_fn_t realloc_fn = hm_realloc_fn(ptr);
        (void)realloc_fn(hm_bucket_ptr(ptr), 0);
        (void)realloc_fn(hm_info_ptr(ptr), 0);
    }
}

#define hm_free(ptr) _hm_free(ptr),ptr=NULL

#define hm_init(ptr, num_items, realloc_fn, hash_func) ptr = hm_bare_realloc(NULL, realloc_fn, hash_func, num_items, sizeof(*ptr))

bool hm_slot_empty(uintptr_t index){
    return index == DEX_TS;
}

bool hm_val_empty(uint8_t val_meta, uint8_t slot_num){
    uint8_t mask = 1 << slot_num;
    return (mask & val_meta) == 0;
}

// return UINT8_MAX on error
uint8_t hm_val_meta_to_open_i(uint8_t input){
    // a zero bit means the slot's open
    if (input == UINT8_MAX) { return UINT8_MAX; }
    uint8_t slot_i = 0;
    for (; (input & 1) == 1; input >>= 1,++slot_i){ }

    return slot_i;
}

uint8_t highest_set_bit(uint8_t n){
    n |= (n >> 1);
    n |= (n >> 2);
    n |= (n >> 4);
    return n - (n >> 1);
}

uint8_t highest_set_bit_i(uint8_t input){
    uint8_t top_bit_set = highest_set_bit(input);
    uint8_t ret = 0;
    if (top_bit_set == 0) return 0;

    for (; (top_bit_set & 1) == 0; top_bit_set>>=1,++ret){ }

    return ret;
}


bool hm_val_slot_open(uint8_t val_meta){
    return val_meta < UINT8_MAX;
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
    uintptr_t hash, truncated_hashes[PROBE_TRIES]; // save the hashes for the value search loop
    uintptr_t cap_mask = hm_cap(ptr) - 1;
    for (; i < PROBE_TRIES; ++i){
        hash = hash_fn(i == 0 ? &key : &hash, sizeof(uintptr_t));
        truncated_hashes[i] = hash & cap_mask;
        uintptr_t bucket_i; uint8_t key_i;
        one_i_to_bucket_is(truncated_hashes[i], bucket_i, key_i);
        hash_bucket *bucket = buckets + bucket_i;

        uint8_t j = key_i, times = GROUP_SIZE, grp_mask = GROUP_SIZE - 1;
        for (; times > 0; --times, j = (j + 1) & grp_mask){
            if (find_empty){
                // look through the slot meta to find an empty slot.
                // We don't need to look through every slot in the bucket since
                // the slot_meta array is not a bucket
                // search the bucket and see if we can insert
                // look for the key
                if (bucket->indices[j] == DEX_TS){
                    bucket_is_to_one_i(key_ret_i, bucket_i, j);
                    goto val_search;
                }
            } else {
                // search the bucket and see if we can insert
                // look for the key
                if (bucket->keys[j] == key && 
                        bucket->indices[j] != DEX_TS){
                    if (dex_slot_out != NULL) { *dex_slot_out = bucket->indices[j]; }
                    bucket_is_to_one_i(key_ret_i, bucket_i, j);
                    return key_ret_i;
                }
            }
        }
    }
    if (i == PROBE_TRIES || key_ret_i == UINTPTR_MAX){ return UINTPTR_MAX; }

val_search:
    ;
    // start looking through everything for a val slot
    // use the old values of bucket_i and key_i
    if (dex_slot_out != NULL && find_empty){
        uint8_t * val_metas = hm_val_meta_ptr(ptr);
        for (uint8_t j = 0; *dex_slot_out == UINTPTR_MAX; ++j){
            // use the stored hash if we have onw
            uintptr_t main_i;
            if (j <= i){
                main_i = truncated_hashes[j]; 
            } else {
                hash = hash_fn(&hash, sizeof(hash));
                main_i = truncate_to_cap(ptr, hash);
            }
            uintptr_t val_i; uint8_t val_bit_i;
            one_i_to_val_is(main_i, val_i, val_bit_i);

            uint8_t slot = hm_val_meta_to_open_i(val_metas[val_i]);
            if (slot != UINT8_MAX){
                val_is_to_one_i(*dex_slot_out, val_i, slot);
                break;
            }
        }
    }

    return key_ret_i;
}

// basically an insert, but we don't need to look for a dex slot
static uintptr_t insert_key_and_dex(void *ptr, uintptr_t key, uintptr_t dex){

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
    buckets[bucket_i].indices[key_i] = dex;
    buckets[bucket_i].keys[key_i] = key;

    return 0;
}

// handle both the init and growing case, but not shrinking yet.
void* hm_bare_realloc(void * ptr, realloc_fn_t realloc_fn, hash_fn_t hash_func, uintptr_t item_count, uintptr_t item_size){

    uint8_t greater_size = (GROUP_SIZE > 8) ? GROUP_SIZE : 8;
    item_count = (item_count < 2*greater_size) ? 2*greater_size : item_count;

    // should be null safe, base_ptr will be null if ptr is null
    hm_info *base_ptr = hm_info_ptr(ptr);

    uintptr_t new_cap = next_pow2(item_count);

    uintptr_t old_num_buckets = hm_cap(ptr)/GROUP_SIZE;
    uintptr_t old_num_val_metas = hm_cap(ptr)/8;
    uintptr_t num_buckets = (new_cap + (GROUP_SIZE-1))/GROUP_SIZE;
    uintptr_t bucket_size = num_buckets*sizeof(hash_bucket);
    uintptr_t data_size = new_cap*item_size + sizeof(hm_info);

    hm_info * inf_ptr = realloc_fn(base_ptr, data_size);
    if (inf_ptr == NULL){
        hm_set_err(ptr, ds_alloc_fail);
        // old pointer is still good, return that
        return ptr;
    }

    uintptr_t num_val_metas = (new_cap + 7)/8;
    // Don't use base_ptr->val_metas, That can get zeroed out after it's reallocated
    uint8_t *old_val_metas = (base_ptr == NULL) ? NULL : inf_ptr->val_metas;
    uint8_t *new_val_metas = realloc_fn(old_val_metas, num_val_metas);
    if (new_val_metas == NULL){
        ++inf_ptr;
        hm_set_err(inf_ptr, ds_alloc_fail);
        return inf_ptr;
    }

    inf_ptr->val_metas = new_val_metas;

    // zero out new val_meta portion to 
    for (uintptr_t i = old_num_val_metas; i < num_val_metas; ++i){
        inf_ptr->val_metas[i] = 0;
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
    inf_ptr->outside_mem = false;

    if (base_ptr == NULL){
        //allocating new array
        inf_ptr->num = 0;
        inf_ptr->realloc_fn = realloc_fn;
        inf_ptr->hash_func = hash_func;
    }

    // set the new meta to empty
    for (uintptr_t i = 0; i < num_buckets; ++i){
        for (uint16_t j = 0; j < GROUP_SIZE; ++j){
            inf_ptr->buckets[i].indices[j] = DEX_TS;
        }
    }
    ++inf_ptr;

    // we should be done if we are just allocating
    if (base_ptr == NULL){
        return inf_ptr;
    }

    // TODO re-insert keys, but leave the indexes since they're okay
    uintptr_t num_items = hm_num(inf_ptr);
    // search the old key structure for keys
    for (uintptr_t bucket_i = 0; bucket_i < old_num_buckets; ++bucket_i){
        for (uint8_t i = 0; i < GROUP_SIZE; ++i){
            if (old_bucket_ptr[bucket_i].indices[i] != DEX_TS){
                // hash the key here and re-insert it into the new array.
                uintptr_t key = old_bucket_ptr[bucket_i].keys[i];
                uintptr_t dex = old_bucket_ptr[bucket_i].indices[i];

                uintptr_t ret = insert_key_and_dex(inf_ptr, key, dex);
                // upon error, revert the indices and return a failure
                if (ret == UINTPTR_MAX){
                    hm_info_ptr(inf_ptr)->buckets = old_bucket_ptr;
                    // free the old memory
                    (void)realloc_fn(bucket_ptr, 0);
                    goto done;
                } 
                --num_items;
                if (num_items == 0){
                    goto free_old_bucket;
                }
            }
        }
    }

    // success, free old buckets
free_old_bucket:
    (void)realloc_fn(old_bucket_ptr, 0);

done:
    hm_set_err(inf_ptr, ds_success);
    return inf_ptr;
}

#define hm_realloc(ptr, new_cap) ptr = hm_bare_realloc(ptr, hm_realloc_fn(ptr), hm_hash_func(ptr), new_cap, sizeof(*ptr))

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
    if (buckets[bucket_i].indices[key_i] == DEX_TS){
        hm_info_ptr(ptr)->num++;
        // set that the slot is taken
    }
    buckets[bucket_i].keys[key_i] = key;
    buckets[bucket_i].indices[key_i] = val_dex;

    bit_set_or_clear(hm_val_meta_ptr(ptr), val_dex, true);

    return val_dex;
}

#define hm_set(ptr, k, v)\
    do{\
        for (uint8_t __hm_grow_tries = 3; __hm_grow_tries > 0; --__hm_grow_tries){\
            if (!should_grow(ptr)){ \
                hm_info_ptr(ptr)->tmp_val_i = hm_raw_insert_key(ptr, k);\
                if (hm_info_ptr(ptr)->tmp_val_i != UINTPTR_MAX){\
                    ptr[hm_info_ptr(ptr)->tmp_val_i] = v;\
                    hm_set_err(ptr, ds_success); \
                    break;\
                } else { \
                    hm_set_err(ptr, ds_not_found); \
                } \
            } \
            hm_realloc(ptr, hm_cap(ptr)+1);\
        }\
    }while(0)

static uintptr_t hm_find_val_i(void *ptr, uintptr_t key){

    uintptr_t key_dex = key_find_helper(
            ptr,
            key,
            NULL,
            false);

    if (key_dex == UINTPTR_MAX){ 
        hm_set_err(ptr, ds_not_found);
        return UINTPTR_MAX; 
    }

    uintptr_t key_bucket; uint8_t key_i;
    one_i_to_bucket_is(key_dex, key_bucket, key_i);
    return hm_bucket_ptr(ptr)[key_bucket].indices[key_i];
}

#define hm_get(ptr, key, val_to_set)\
    do {\
        hm_info_ptr(ptr)->tmp_val_i = hm_find_val_i(ptr, key);\
        if (hm_info_ptr(ptr)->tmp_val_i != UINTPTR_MAX){\
            val_to_set = ptr[hm_info_ptr(ptr)->tmp_val_i];\
        }\
    } while(0)

// TODO: STB just puts the newest element at the end of the array and memmoves 
// old elements over the removed one. Implement that so we can get rid of val_meta
// We don't need to really worry about it right now since queries are about as
// fast as STB's
void hm_del(void *ptr, uintptr_t key){

    uintptr_t val_dex, key_dex = key_find_helper(
            ptr,
            key,
            &val_dex,
            false);

    if (key_dex == UINTPTR_MAX){
        hm_set_err(ptr, ds_not_found);
        return;
    }

    uintptr_t bucket_i; uint8_t key_i;
    one_i_to_bucket_is(key_dex, bucket_i, key_i);

    hash_bucket * buckets = hm_bucket_ptr(ptr);

    bit_set_or_clear(hm_val_meta_ptr(ptr), buckets[bucket_i].indices[key_i], false);

    buckets[bucket_i].indices[key_i] = DEX_TS;
    hm_set_err(ptr, ds_success);
}
