#pragma once
#include "dynarr.h"
#include "bit_setting.h"

// tombstone (empty) marker
#define DEX_TS ((uintptr_t)UINTPTR_MAX)

// used to find remap entries
#define HASH_MULT_COEFF (2654435769)

uintptr_t hm_jump_dist(uint8_t in){
    uintptr_t top_half = (in & 0xf0) >> 4;
    return in << top_half | in;
}

#define GROUP_SIZE (8)

#define one_i_to_two(main_i, bucket_i, key_i) bucket_i = (main_i)/GROUP_SIZE; key_i = main_i - (bucket_i*GROUP_SIZE)
#define two_i_to_one(main_i, bucket_i, key_i) (main_i) = bucket_i*GROUP_SIZE + key_i

#define RND_TO_GRP_NUM(x) ((x + (GROUP_SIZE-1))/GROUP_SIZE)
typedef struct {
    uint8_t jump_dists[GROUP_SIZE];// distance to next item, 0 for no more items
    // keys, indices for values, and an index for a next bucket to check
    uintptr_t keys[GROUP_SIZE], indices[GROUP_SIZE], remap_i; 
    uint8_t val_meta; // bit set for value taken
    uint8_t direct_hit; // if the key is a direct hit or not set for direct, hit, cleared otherwise
    uint8_t key_type; // 0 for numeric key, 1 for c string key.
    uint8_t num; // how many keys are in the bucket
} hash_bucket;

// hash function prototype
typedef uintptr_t (*hash_fn_t)(void *, size_t);

typedef struct hm_info{
    hash_fn_t hash_func;
    realloc_fn_t realloc_fn;
    // holds the metadata for the hash table.
    hash_bucket* buckets;
    uintptr_t cap,num;
    uint8_t err,outside_mem;
} hm_info;

hm_info * hm_info_ptr(void * ptr){
    return (ptr == NULL) ? NULL : (hm_info*)ptr - 1;
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
        void* __absorb_realloc__ptr = realloc_fn(hm_bucket_ptr(ptr), 0);
        __absorb_realloc__ptr = realloc_fn(hm_info_ptr(ptr), 0);
        (void)__absorb_realloc__ptr;
    }
}

#define hm_free(ptr) _hm_free(ptr),ptr=NULL

void *_hm_init(size_t num_items, size_t item_size, realloc_fn_t realloc_fn, hash_fn_t hash_func){

    // round up to 2 buckets
    num_items = (num_items < GROUP_SIZE*2) ? GROUP_SIZE*2 : num_items;

    hm_info *ret_ptr = realloc_fn(NULL, num_items*item_size + sizeof(hm_info));
    if (ret_ptr == NULL) {return NULL;}

    // don't set cap until we have buckets allocated
    ret_ptr->outside_mem = false;
    ret_ptr->num = 0;
    ret_ptr->realloc_fn = realloc_fn;
    ret_ptr->hash_func = hash_func;

    // round up to the number of buckets needed
    uintptr_t num_buckets = (num_items + (GROUP_SIZE - 1))/GROUP_SIZE;
    hash_bucket *buckets = realloc_fn(NULL, num_buckets*sizeof(hash_bucket));
    if (buckets == NULL){
        ret_ptr->err = ds_alloc_fail;
        ret_ptr->cap = 0;
        return NULL;
    }

    ret_ptr->cap = num_items;
    ret_ptr->buckets = buckets;

    // init all the buckets
    for (uintptr_t i = 0; i < num_buckets; ++i){
        // set the TS value for indices
        for (uint8_t j = 0;  j < GROUP_SIZE; ++j){
            buckets[i].indices[j] = DEX_TS;
            buckets[i].jump_dists[j] = 0;
        }
        buckets[i].remap_i = UINTPTR_MAX;
        buckets[i].val_meta = 0;
        buckets[i].direct_hit = 0;
        buckets[i].key_type = 0;
        buckets[i].num = 0;
    }

    ++ret_ptr;
    return ret_ptr;
}

#define hm_init(ptr, num_items, realloc_fn, hash_func) ptr = _hm_init(num_items, sizeof(*ptr), realloc_fn, hash_func)

// API:
// - find (key) -> index
// - set (key, val) -> err
// - get (key) -> val
// - set_cap

bool hm_slot_empty(uintptr_t index){
    return index == DEX_TS;
}

bool hm_val_empty(uint8_t val_meta, uint8_t slot_num){
    uint8_t mask = 1 << slot_num;
    return (mask & val_meta) == 0;
}

// bit val of 0 means direct hit
bool hm_slot_direct_hit(uint8_t key_meta, uint8_t slot_num){
    uint8_t key_mask = 1 << slot_num;
    return (key_meta & key_mask) == 0;
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

// returns the dex in the bucket array where the key is placed 
static uintptr_t find_slot(void *ptr, uintptr_t key, bool look_for_slot){
    return UINTPTR_MAX;
}

// basically an insert, but we don't need to look for a dex slot
static uintptr_t insert_key_and_dex(void *ptr, uint64_t key, uintptr_t dex){

    if (hm_num(ptr) == hm_cap(ptr)){ return UINTPTR_MAX; }

    // find a bucket that's not full
    uint64_t hash = hm_hash_func(ptr)(&key, sizeof(key));
    uintptr_t truncated_hash = truncate_to_cap(ptr, hash);
    bool direct_hit = true;
    uintptr_t index = 0;
    uint8_t total_tries = 4, tries;
    uintptr_t step = 1;
    hash_bucket *buckets = hm_bucket_ptr(ptr);
    uintptr_t bucket_i; uint8_t key_i;
    one_i_to_two(truncated_hash, bucket_i, key_i);

    // if this bucket has a list attached to it, then get to the end of it
    while (buckets[bucket_i].keys[key_i] != key &&
           buckets[bucket_i].indices[key_i] != DEX_TS &&
           buckets[bucket_i].jump_dists[key_i] > 0){

        uintptr_t jump_dist = hm_jump_dist(buckets[bucket_i].jump_dists[key_i]);

        uintptr_t new_dex = truncate_to_cap(
            ptr,
            bucket_i*GROUP_SIZE + key_i + jump_dist);
        one_i_to_two(new_dex, bucket_i, key_i);
        direct_hit = false;
    }

    // if the key matches, then pass out the index we already have
    if (buckets[bucket_i].keys[key_i] == key && buckets[bucket_i].indices[key_i] != DEX_TS){
            index = buckets[bucket_i].indices[key_i];
    } else if (buckets[bucket_i].indices[key_i] == DEX_TS){
        // set the key
        buckets[bucket_i].keys[key_i] = key;
    } else if (buckets[bucket_i].jump_dists[key_i] == 0){
        //scan for openings
        uint8_t jump_i;
        // don't include UINT8_MAX to keep from causing a int overflow 
        for (jump_i = 1; jump_i < UINT8_MAX; ++jump_i){
            uintptr_t dex_start;
            two_i_to_one(dex_start, bucket_i, key_i);
            dex_start = truncate_to_cap(ptr, dex_start + hm_jump_dist(jump_i));

            uintptr_t new_bucket_i; uint8_t new_key_i;
            one_i_to_two(dex_start, new_bucket_i, new_key_i);

            // only check for the tombstone since we shouldn't find the key this way
            if (buckets[new_bucket_i].indices[new_key_i] == DEX_TS){
                // set the key bub
                buckets[new_bucket_i].keys[new_key_i] = key;

                // set the other entry to redirect to here
                buckets[bucket_i].jump_dists[key_i] = jump_i;
                break;
            }
        }
        // give up
        if (jump_i == UINT8_MAX){
            return UINTPTR_MAX;
        }
    }

    return 0;
}

void* hm_bare_realloc(void * ptr, uintptr_t item_count, uintptr_t item_size){

    item_count = (item_count < 2*GROUP_SIZE) ? 2*GROUP_SIZE : item_count;

    hm_info *base_ptr = hm_info_ptr(ptr);

    uintptr_t old_cap = hm_cap(ptr);
    uintptr_t new_cap = next_pow2(item_count);

    uintptr_t old_num_buckets = hm_cap(ptr)/GROUP_SIZE;
    uintptr_t num_buckets = (new_cap + (GROUP_SIZE-1))/GROUP_SIZE;
    uintptr_t bucket_size = num_buckets*sizeof(hash_bucket);
    uintptr_t data_size = new_cap*item_size + sizeof(hm_info);

    realloc_fn_t realloc_fn = hm_realloc_fn(ptr);

    hm_info * inf_ptr = realloc_fn(base_ptr, data_size);
    if (inf_ptr == NULL){
        hm_set_err(ptr, ds_alloc_fail);
        // old pointer is still good, return that
        return ptr;
    }

    hash_bucket *old_bucket_ptr = inf_ptr->buckets;
    hash_bucket *bucket_ptr = realloc_fn(NULL, bucket_size);
    if (bucket_ptr == NULL){
        hm_set_err(ptr, ds_alloc_fail);
        return ptr;
    }

    // associate the key pointer with the new structure
    inf_ptr->buckets = bucket_ptr;
    inf_ptr->cap = new_cap;
    inf_ptr->outside_mem = false;

    if (base_ptr == NULL){

        // set all the key metadata to empty
        for (uintptr_t i = 0; i < num_buckets; ++i){
            for (uint16_t j = 0; j < GROUP_SIZE; ++j){
                inf_ptr->buckets[i].indices[j] = DEX_TS;
                inf_ptr->buckets[i].jump_dists[j] = 0;
            }
            inf_ptr->buckets[i].val_meta = 0;
            inf_ptr->buckets[i].num = 0;
            inf_ptr->buckets[i].direct_hit = 0;
            inf_ptr->buckets[i].key_type = 0;
            inf_ptr->buckets[i].remap_i = UINTPTR_MAX;
        }
        inf_ptr->err = ds_success;
        inf_ptr->num = 0;
    } else {
        // set the new meta to empty
        for (uintptr_t i = 0; i < num_buckets; ++i){
            for (uint16_t j = 0; j < GROUP_SIZE; ++j){
                inf_ptr->buckets[i].indices[j] = DEX_TS;
                inf_ptr->buckets[i].jump_dists[j] = 0;
            }
            inf_ptr->buckets[i].remap_i = UINTPTR_MAX;
            // keep the which value slots are taken
            if (i < old_cap){
                inf_ptr->buckets[i].val_meta = old_bucket_ptr[i].val_meta;
            } else {
                inf_ptr->buckets[i].val_meta = 0;
            }
            inf_ptr->buckets[i].direct_hit = 0;
            inf_ptr->buckets[i].key_type = 0;
            inf_ptr->buckets[i].num = 0;
        }
        ++inf_ptr;

        // TODO re-insert keys, but leave the indexes since they're okay
        uintptr_t num_items = hm_num(inf_ptr);
        // search the old key structure for keys
        for (uintptr_t bucket_i = 0; bucket_i < old_num_buckets; ++bucket_i){
            for (uint8_t i = 0; i < GROUP_SIZE; ++i){
                if (old_bucket_ptr[bucket_i].indices[i] != DEX_TS){
                    // hash the key here and re-insert it into the new array.
                    uint64_t key = old_bucket_ptr[bucket_i].keys[i];
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
                        goto done;
                    }
                }
            }
        }
    }

    // success, free old buckets
    (void)realloc_fn(old_bucket_ptr, 0);

done:
    hm_set_err(inf_ptr, ds_success);
    return inf_ptr;
}

#define hm_realloc(ptr, new_cap) ptr = hm_bare_realloc(ptr, new_cap, sizeof(*ptr))

// returns the value index
// sets the key slot found
// dex out is the index where the key is in the the bucket array
uintptr_t hm_raw_insert_key(
        void *ptr, 
        uint64_t key,
        uintptr_t *key_dex_out)
{
    if (hm_num(ptr) == hm_cap(ptr)){
        return UINTPTR_MAX;
    }

    // hash the key and start looking for slots in the bucket it hashes to
    uintptr_t hash = hm_hash_func(ptr)(&key, sizeof(key));
    // true if key was replaced
    bool replace = false;

    uintptr_t truncated_hash = truncate_to_cap(ptr, hash);
    uintptr_t bucket_i; uint8_t key_i;
    one_i_to_two(truncated_hash, bucket_i, key_i);
    uintptr_t ret_index = DEX_TS;
    *key_dex_out = UINTPTR_MAX;

    hash_bucket* buckets = hm_bucket_ptr(ptr);
    bool direct_hit = true;
    uintptr_t prev_dex;

    // if this bucket has a list attached to it, then get to the end of it
    while (buckets[bucket_i].keys[key_i] != key &&
           buckets[bucket_i].indices[key_i] != DEX_TS &&
           buckets[bucket_i].jump_dists[key_i] > 0){

        // look for a value slot
        if (ret_index == DEX_TS){
            uint8_t slot = hm_val_meta_to_open_i(buckets[bucket_i].val_meta);
            if (slot != UINT8_MAX){
                ret_index = bucket_i*GROUP_SIZE + slot;
            }
        }
        two_i_to_one(prev_dex, bucket_i, key_i);
        uintptr_t jump_dist = hm_jump_dist(buckets[bucket_i].jump_dists[key_i]);

        uintptr_t new_dex = truncate_to_cap(
            ptr,
            bucket_i*GROUP_SIZE + key_i + jump_dist);
        one_i_to_two(new_dex, bucket_i, key_i);
        direct_hit = false;
    }
    //
    // if the key matches, then pass out the index we already have
    if (buckets[bucket_i].keys[key_i] == key && 
        buckets[bucket_i].indices[key_i] != DEX_TS){
        printf("[debug] key replace %u\n", key);
        two_i_to_one(*key_dex_out, bucket_i, key_i);
        replace = true;
    } else if (buckets[bucket_i].indices[key_i] == DEX_TS){
        // set the key
        printf("[debug] key set %u\n", key);
        buckets[bucket_i].keys[key_i] = key;
        two_i_to_one(*key_dex_out, bucket_i, key_i);
    } else if (buckets[bucket_i].jump_dists[key_i] == 0){
        //scan for openings
        uint8_t jump_i;
        // don't include UINT8_MAX to keep from causing a int overflow 
        printf("[debug] key traverse %u\n", key);
        for (jump_i = 1; jump_i < UINT8_MAX; ++jump_i){
            uintptr_t dex_start;
            two_i_to_one(dex_start, bucket_i, key_i);
            dex_start = truncate_to_cap(ptr, dex_start + hm_jump_dist(jump_i));

            uintptr_t new_bucket_i; uint8_t new_key_i;
            one_i_to_two(dex_start, new_bucket_i, new_key_i);

            // keep look for a value slot
            if (ret_index == DEX_TS){
                uint8_t slot = hm_val_meta_to_open_i(buckets[new_bucket_i].val_meta);
                if (slot != UINT8_MAX){
                    ret_index = new_bucket_i*GROUP_SIZE + slot;
                }
            }

            // only check for the tombstone since we shouldn't find the key this way
            if (buckets[new_bucket_i].indices[new_key_i] == DEX_TS){
                // set the key bub
                buckets[new_bucket_i].keys[new_key_i] = key;
                //set this entry to be the end of the linked list
                buckets[new_bucket_i].jump_dists[new_key_i] = 0;
                two_i_to_one(*key_dex_out, new_bucket_i, new_key_i);

                // set the other entry to redirect to here
                buckets[bucket_i].jump_dists[key_i] = jump_i;
                break;
            }
        }
        // give up
        if (jump_i == UINT8_MAX){
            return UINTPTR_MAX;
        }
    }

    // start looking through everything (linear search) for a val slot
    for (uintptr_t i = 0, num_buckets = hm_cap(ptr)/GROUP_SIZE;
         i < num_buckets && ret_index == DEX_TS; ++i){
        uint8_t slot = hm_val_meta_to_open_i(buckets[i].val_meta);
        if (slot != UINT8_MAX){
            ret_index = i*GROUP_SIZE + slot;
        }
    }

    // only set the index of the data when we know we have a slot to store the data with
    if (ret_index != DEX_TS){
        uintptr_t val_bucket_i; uint8_t val_key_i;
        one_i_to_two(ret_index, val_bucket_i, val_key_i);
        buckets[val_bucket_i].val_meta |= 1 << val_key_i; 
    } else {
        return UINTPTR_MAX;
    }

    if (!replace){
        hm_info_ptr(ptr)->num++;
    }
    // set the key meta if we should

    uintptr_t final_bucket_i; uint8_t final_key_i;
    one_i_to_two(*key_dex_out, final_bucket_i, final_key_i);
    bit_set_or_clear(&(buckets[final_bucket_i].direct_hit), final_key_i, direct_hit); 
    return ret_index;
}

#define hm_set(ptr, k, v)\
    do{\
        for (uint8_t __hm_grow_tries = 2; __hm_grow_tries > 0; --__hm_grow_tries){\
            uintptr_t _hm_slot_i = UINTPTR_MAX;\
            uintptr_t __ds_empty_slot = hm_raw_insert_key(ptr, k, &_hm_slot_i);\
            if (__ds_empty_slot != UINTPTR_MAX){\
                printf("key=%lu, k_slot=%lu\n",k, _hm_slot_i);\
                uintptr_t _hm_bucket_i; uint8_t _hm_key_i;\
                one_i_to_two(_hm_slot_i, _hm_bucket_i, _hm_key_i);\
                ptr[__ds_empty_slot] = v;\
                hm_bucket_ptr(ptr)[_hm_bucket_i].indices[_hm_key_i] = __ds_empty_slot;\
                hm_set_err(ptr, ds_success); \
                break;\
            } else { \
                hm_set_err(ptr, ds_not_found); \
            } \
            hm_realloc(ptr, hm_cap(ptr)+1);\
        }\
    }while(0)

uintptr_t hm_find_val_i(void *ptr, uintptr_t key){
    uintptr_t hash = hm_hash_func(ptr)(&key, sizeof(key));
    uintptr_t truncated_hash = truncate_to_cap(ptr, hash);
    uintptr_t bucket_i; uint8_t key_i;
    one_i_to_two(truncated_hash, bucket_i, key_i);
    uint8_t orig_key_i = key_i;

    hash_bucket* buckets = hm_bucket_ptr(ptr);

    // if this bucket has a list attached to it, then get to the end of it
    while (buckets[bucket_i].keys[key_i] != key &&
           buckets[bucket_i].indices[key_i] != DEX_TS &&
           buckets[bucket_i].jump_dists[key_i] > 0){

        uintptr_t jump_dist = hm_jump_dist(buckets[bucket_i].jump_dists[key_i]);
        uintptr_t new_dex = truncate_to_cap(
            ptr,
            bucket_i*GROUP_SIZE + key_i + jump_dist);
        one_i_to_two(new_dex, bucket_i, key_i);
    }
    if (buckets[bucket_i].keys[key_i] == key && buckets[bucket_i].indices[key_i] != DEX_TS){
        return buckets[bucket_i].indices[key_i];
    } 
    // no active key here
    return UINTPTR_MAX;
}

#define hm_get(ptr, key, val_to_set)\
    do {\
        uintptr_t __val_i = hm_find_val_i(ptr, key);\
        if (__val_i != UINTPTR_MAX){\
            val_to_set = ptr[__val_i];\
        }\
    } while(0)

void hm_del(void *ptr, uintptr_t key){

    // hash the key and start looking for slots in the bucket it hashes to
    uint64_t hash = hm_hash_func(ptr)(&key, sizeof(key));

    uintptr_t truncated_hash = truncate_to_cap(ptr, hash);
    uintptr_t bucket_i = truncated_hash;

    hash_bucket *buckets = hm_bucket_ptr(ptr);

    // find the bucket for this key and then see if the key is in the bucket
    do {
        // search the bucket, see if the key is there
        for (uint8_t i = 0; i < GROUP_SIZE; ++i){
            if (buckets[bucket_i].indices[i] != DEX_TS &&
                buckets[bucket_i].keys[i] == key){
                
                // set the index to the TS
                buckets[bucket_i].indices[i] = DEX_TS;

                // found the key for the val
                hm_set_err(ptr, ds_success);
                return;
            }
        }

        // if you don't find it, check for a REMAP entry and see if there's another
        // bucket to check
        if (buckets[bucket_i].remap_i != UINTPTR_MAX){
            bucket_i = buckets[bucket_i].remap_i;
        } else {
            hm_set_err(ptr, ds_not_found);
            return;
        }
    } while (true);
}
