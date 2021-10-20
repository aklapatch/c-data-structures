#pragma once
#include "dynarr.h"
#include "ahash.h"

// tombstone (empty) marker
#define DEX_TS ((uintptr_t)UINTPTR_MAX)

// REMAP maker. In this case, the key holds the next bucket to go to
#define DEX_REMAP ((uintptr_t)UINTPTR_MAX - 1)

// used to find remap entries
#define HASH_MULT_COEFF (2654435769)

// switch to horton table
// keep keys and indices
// maybe ditch list_meta
// for horton tables, the last entry gets converted to a remap entry if the bucket fills up
// keys hash to buckets (need to see what I want to do regarding the hash bits (truncate/keep))
// usual horton tables have a second hash to jump another bucket, but I want to do something different if possible
// If I can't do something different, then I can just re-hash the untruncated hash of the key.
// i still need val_meta since values underneath are still up for grabs
// The searching strategy for value slots will be simple, search the bucket, then  move onto the next bucket and search that
// use all the hash bits to hash to a bucket (buckets must be pow2, which sounds really bad, and probably is)
// Actually, I think we'll be fine. # of buckets will always be pow2/8 which should still be a pow2 (except if size is 8, which is not necessary)
// UINTPTR_MAX -1 can be the sentinel value for  a remap entry 
// On insertion, check the end entry and if it is a remap entry, then it's full.
#define GROUP_SIZE (8)
typedef struct {
    uint64_t keys[GROUP_SIZE]; // when you have a remap entry, then 
    uintptr_t indices[GROUP_SIZE]; // when the bucket is full, this becomes a remap entry
    uintptr_t remap_i; // The index of the next bucket to check 
    uint8_t val_meta; // bit set for value taken
    uint8_t key_meta; 
    uint8_t num; // how many keys are in the bucket
} hash_bucket;

/// Too complex for now
// we can use the top bit of the index member to signify if the value slot it open. 
// Then there are a couple values we can use as markers
// top bit of number (1 << (sizeof(uintptr_t)*8 - 1)) mask for uintptr_t
// if bit is set then the slot is taken
// Be careful to |= the numbers when assigning them., and to clear them properly

typedef struct hm_info{
    // holds the metadata for the hash table.
    hash_bucket* buckets;
    uintptr_t cap,num;
    uint8_t err,outside_mem,hash_table;
} hm_info;

hm_info * hm_info_ptr(void * ptr){
    return (ptr == NULL) ? NULL : (hm_info*)ptr - 1;
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
    return num & ((hm_cap(ptr)/GROUP_SIZE) - 1);
}

#define RND_TO_GRP_NUM(x) ((x + (GROUP_SIZE-1))/GROUP_SIZE)

// The meta table in the info struct needs to be allocated too
void* hm_bare_realloc(void * ptr,uintptr_t item_count, uintptr_t item_size){

    item_count = (item_count < 16) ? 16 : item_count;

    hm_info *base_ptr = hm_info_ptr(ptr);

    uintptr_t new_cap = next_pow2(item_count);

    uintptr_t num_buckets = (new_cap + (GROUP_SIZE-1))/GROUP_SIZE;
    uintptr_t bucket_size = num_buckets*sizeof(hash_bucket);
    uintptr_t data_size = new_cap*item_size + sizeof(hm_info);

    hm_info * inf_ptr = C_DS_REALLOC(base_ptr, data_size);
    if (inf_ptr == NULL){
        hm_set_err(ptr, ds_alloc_fail);
        return ptr;
    }

    hash_bucket* bucket_ptr = C_DS_REALLOC(inf_ptr->buckets, bucket_size);
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
            }
            inf_ptr->buckets[i].val_meta = 0;
            inf_ptr->buckets[i].key_meta = 0;
            inf_ptr->buckets[i].remap_i = UINTPTR_MAX;
        }
        inf_ptr->err = ds_success;
        inf_ptr->num = 0;
    } else {
        // set the new meta to empty
        uintptr_t old_len = hm_cap(ptr);
        for (uintptr_t i = old_len/GROUP_SIZE; i < num_buckets; ++i){
            for (uint16_t j = 0; j < GROUP_SIZE; ++j){
                inf_ptr->buckets[i].indices[j] = DEX_TS;
            }
            inf_ptr->buckets[i].val_meta = 0;
            inf_ptr->buckets[i].key_meta = 0;
        }

        // TODO re-insert keys, but leave the indexes since they're okay


        // free old memory
        C_DS_FREE(hm_bucket_ptr(ptr));
        C_DS_FREE(base_ptr);
    }
    return ++inf_ptr;
}

#define hm_realloc(ptr, new_cap) ptr = hm_bare_realloc(ptr, new_cap, sizeof(*ptr))

// returns the value index
// sets the key slot found
// also returns whether this is a secondary or primary hit
uintptr_t hm_raw_insert_key(
        void *ptr, 
        uint64_t key)
{
    if (hm_num(ptr) == hm_cap(ptr)){
        return UINTPTR_MAX;
    }

    // hash the key and start looking for slots in the bucket it hashes to
    uint64_t hash = C_DS_HASH_FUNC((uint8_t*)&key,sizeof(key));

    uintptr_t truncated_hash = truncate_to_cap(ptr, hash);
    uintptr_t bucket_i = truncated_hash;
    uintptr_t ret_index = DEX_TS;
    uintptr_t key_final_bucket_i = UINTPTR_MAX;
    uint8_t in_bucket_i = UINT8_MAX;

    hash_bucket* buckets = hm_bucket_ptr(ptr);
    bool direct_hit = true;

    // look through the bucket for a TS so we can insert a key
    uint8_t total_tries = 4, tries;
    uintptr_t step = 1;
    for (tries = total_tries; tries > 0; --tries){

        // find a bucket that's not full
        while (buckets[bucket_i].indices[GROUP_SIZE - 1] == DEX_REMAP){
            // look for empty value slots while we're here
            if (ret_index == DEX_TS){
                uint8_t slot = hm_val_meta_to_open_i(buckets[bucket_i].val_meta);
                if (slot != UINT8_MAX){
                    ret_index = bucket_i*GROUP_SIZE + slot;
                }
            }
            bucket_i = buckets[bucket_i].keys[GROUP_SIZE - 1];
            direct_hit = false;
        }
        if (ret_index == DEX_TS){
            uint8_t slot = hm_val_meta_to_open_i(buckets[bucket_i].val_meta);
            if (slot != UINT8_MAX){
                ret_index = bucket_i*GROUP_SIZE + slot;
            }
        }

        uint8_t i;
        for (i = 0; i < GROUP_SIZE; ++i){
            if (buckets[bucket_i].indices[i] == DEX_TS){
                // set the key
                buckets[bucket_i].keys[i] = key;

                // clear the meta bit for this entry
                if (direct_hit){
                    // only clear one meta bit
                    uint8_t meta_mask = ~(uint8_t)(1 << i);
                    buckets[bucket_i].key_meta &= meta_mask;
                } else {
                    buckets[bucket_i].key_meta |= 1 << i ;
                }
                break;
            }
        }
        // done if we insert a key
        if (buckets[bucket_i].keys[i] == key){
            in_bucket_i = i;
            key_final_bucket_i = bucket_i;
            break;
        }

        // probe for a new bucket to check
        uintptr_t new_bucket_i = bucket_i + step;
        step += 1;
        bucket_i = truncate_to_cap(ptr, new_bucket_i);
        direct_hit = false;
    }
    // give up
    if (tries == 0){
        return UINTPTR_MAX;
    }
    // start looking through everything (linear search)
    for (uintptr_t i = 0, num_buckets = hm_cap(ptr)/GROUP_SIZE;
         i < num_buckets && ret_index == DEX_TS; ++i){
        uint8_t slot = hm_val_meta_to_open_i(buckets[i].val_meta);
        if (slot != UINT8_MAX){
            ret_index = i*GROUP_SIZE + slot;
        }
    }

    // only set the index of the data when we know we have a slot to store the data with
    if (ret_index != DEX_TS && in_bucket_i != UINT8_MAX && key_final_bucket_i != DEX_TS){
        // set the value slot to show it's taken
        buckets[key_final_bucket_i].indices[in_bucket_i] = ret_index;
        uintptr_t val_bucket = ret_index/GROUP_SIZE;
        buckets[val_bucket].val_meta |= 1 << in_bucket_i; 
    } else {
        return UINTPTR_MAX;
    }

    // if we need to make a REMAP entry, then we need to re-insert the REMAP
    // entry into a new bucket
    if (bucket_i != truncated_hash && buckets[truncated_hash].indices[GROUP_SIZE - 1] != DEX_REMAP){
        uint64_t old_key = buckets[truncated_hash].keys[GROUP_SIZE - 1];
        uintptr_t old_dex = buckets[truncated_hash].indices[GROUP_SIZE - 1];

        // set this to a remap
        buckets[truncated_hash].keys[GROUP_SIZE - 1] = bucket_i;
        buckets[truncated_hash].indices[GROUP_SIZE - 1] = DEX_REMAP;

        // check if we can use this new key at the bucket we just found:
        uint8_t i;
        for (i = 0; i < GROUP_SIZE; ++i){
            if (buckets[bucket_i].indices[i] == DEX_TS){
                // set the key
                buckets[bucket_i].keys[i] = old_key;
                buckets[bucket_i].indices[i] = old_dex;

                // not a direct hit, set as such
                buckets[bucket_i].key_meta |= 1 << i ;
                break;
            }
        }
        // done if we insert a key
        if (buckets[bucket_i].keys[i] == old_key){

        } else {
            // TODO:
            // The hash table's in a pretty bad state if this doesn't work
            // I need to find a better abstraction so I can recurse to 
            // insert keys better.
            return UINTPTR_MAX;
        }
    }

    return ret_index;
}

#define hm_set(ptr, k, v)\
    do{\
        uintptr_t __ds_empty_slot = hm_raw_insert_key(ptr, k);\
        if (__ds_empty_slot != UINTPTR_MAX){\
            ptr[__ds_empty_slot] = v;\
            hm_set_err(ptr, ds_success); \
        } else { \
            hm_set_err(ptr, ds_not_found); \
        } \
    }while(0)

uintptr_t hm_find_val_i(void *ptr, uint64_t key){
    uint64_t hash = C_DS_HASH_FUNC((uint8_t*)&key, sizeof(key));
    uintptr_t truncated_hash = truncate_to_cap(ptr, hash);
    uintptr_t bucket_i = truncated_hash;

    hash_bucket* buckets = hm_bucket_ptr(ptr);

    // find the bucket for this key and then see if the key is in the bucket
    do {
        // search the bucket, see if the key is there
        for (uint8_t i = 0; i < GROUP_SIZE; ++i){
            if (buckets[bucket_i].indices[i] != DEX_TS &&
                buckets[bucket_i].indices[i] != DEX_REMAP &&
                buckets[bucket_i].keys[i] == key){

                // found the key for the val
                hm_set_err(ptr, ds_success);
                return buckets[bucket_i].indices[i];
            }
        }

        // if you don't find it, check for a REMAP entry and see if there's another
        // bucket to check
        if (buckets[bucket_i].indices[GROUP_SIZE - 1] == DEX_REMAP){
            bucket_i = buckets[bucket_i].keys[GROUP_SIZE - 1];
        } else {
            hm_set_err(ptr, ds_not_found);
            return UINTPTR_MAX;
        }
    } while (true);
}

#define hm_get(ptr, key, val_to_set)\
    do{\
        uintptr_t __hm_found_dex = hm_find_val_i(ptr, key);\
        if (__hm_found_dex == UINTPTR_MAX){\
            hm_set_err(ptr, ds_not_found);\
        } else {\
            val_to_set = ptr[__hm_found_dex]; \
            hm_set_err(ptr, ds_success);\
        }\
    }while(0)
