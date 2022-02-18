#pragma once

#include<stdint.h>
#include<stddef.h>
#include<string.h>
#include<stdbool.h>
#include"ds_common.h"

typedef struct dynarr_inf{
    realloc_fn_t realloc_fn;
    uintptr_t num,cap;
    // allocator that the user can pass in
    uint8_t err;
    uint8_t outside_mem;
} dynarr_inf;

dynarr_inf * dynarr_info(void * ptr){
    return (ptr == NULL) ? NULL : ((dynarr_inf*)ptr) - 1;
}

uintptr_t dynarr_num(void *ptr){
    return (ptr == NULL) ? 0 : dynarr_info(ptr)->num;
}

uintptr_t dynarr_cap(void *ptr){
    return (ptr == NULL) ? 0 : dynarr_info(ptr)->cap;
}

realloc_fn_t dynarr_realloc_fn(void *ptr){
    return (ptr == NULL) ? NULL : dynarr_info(ptr)->realloc_fn;
}

bool dynarr_outside_mem(void *ptr){
    return (ptr == NULL) ?  false : dynarr_info(ptr)->outside_mem;
}

void dynarr_set_err(void * ptr, ds_error_e err){
    if (ptr != NULL){
        dynarr_info(ptr)->err = err;
    }
}

ds_error_e dynarr_err(void* ptr){
    return (ptr == NULL) ? ds_null_ptr : dynarr_info(ptr)->err;
}

bool dynarr_is_err_set(void * ptr){
    return dynarr_err(ptr) != ds_success;
}

char * dynarr_err_str(void* ptr){
    return ds_get_err_str(dynarr_err(ptr));
}

void *_dynarr_init(size_t num_elems, size_t elem_size, realloc_fn_t realloc_fn){
    // we're okay with returning the NULL here.
    dynarr_inf *ret_ptr = realloc_fn(NULL, num_elems*elem_size + sizeof(dynarr_inf));
    if (ret_ptr == NULL) {return NULL;}

    ret_ptr->err = ds_success;
    ret_ptr->outside_mem = false;
    ret_ptr->num = 0;
    ret_ptr->cap = num_elems;
    ret_ptr->realloc_fn = realloc_fn;

    ++ret_ptr;
    // increment to get past the meta info and point to the first
    // element
    return ret_ptr;
}

// example usage
// int *i;
// dynarr_init(i, realloc);
// OVERWRITES ptr
#define dynarr_init(ptr, num_elems, realloc_fn) ptr = _dynarr_init(num_elems, sizeof(*(ptr)), realloc_fn)

// TODO: test this better,
void *bare_dynarr_init_from_buf(
        void* buf, 
        uintptr_t buf_size_bytes,
        uintptr_t item_size,
        realloc_fn_t realloc_fn){

    // set up the base of the buffer to be where the info struct is
    uint8_t *byte_ptr = buf, *orig_ptr = buf;
    // handle alignment
    uint16_t mod = (uintptr_t)byte_ptr % sizeof(uintptr_t);
    if (mod > 0){
        byte_ptr += (sizeof(uintptr_t) - mod);
    }
    if (byte_ptr + sizeof(dynarr_inf) > orig_ptr + buf_size_bytes){
        return NULL;
    }
    dynarr_inf *base = (dynarr_inf*)byte_ptr;
    base->num = 0;
    base->err = ds_success;
    base->outside_mem = true;
    base->realloc_fn = realloc_fn;

    buf_size_bytes -= (uintptr_t)(byte_ptr - orig_ptr) + sizeof(dynarr_inf);
    base->cap= buf_size_bytes/item_size;

    ++base;
    return base;
}

#define dynarr_init_from_buf(ptr, buf, buf_size_bytes, realloc_fn) ptr = bare_dynarr_init_from_buf(buf, buf_size_bytes, sizeof(*ptr), realloc_fn)

// This should never be used to free anything because it automatically adds the size of the info struct,
// to the allocated amount.
// I need to figure out how to handle the realloc_fn. If the pointer
// being passed in does not have the realloc_fn attached to it if NULL
// is passed in 
void* bare_dynarr_realloc(void * ptr,uintptr_t item_count, uintptr_t item_size){

    if (ptr == NULL) { return NULL; }

    dynarr_inf *base_ptr = NULL;
    // capture the realloc_fn in case weird things happen while ptr is
    // being realloced
    realloc_fn_t realloc_fn = dynarr_realloc_fn(ptr); 

    // if memory is being provided from the outside, then feed NULL into 
    // realloc instead of an actual pointer.
    bool outside_mem = dynarr_outside_mem(ptr);
    base_ptr = (outside_mem) ? NULL : dynarr_info(ptr);

    uintptr_t old_num = dynarr_num(ptr);

    dynarr_inf *new_ptr = realloc_fn(base_ptr, item_count*item_size + sizeof(dynarr_inf));
    if (new_ptr != NULL){
        new_ptr->cap = item_count;
        if (base_ptr == NULL && !outside_mem){
            new_ptr->num = 0;
        } else {
            // change the num to be lower if the cap is lower than it
            new_ptr->num = (old_num > item_count) ? item_count: old_num;
        }

        new_ptr->err = ds_success;
        new_ptr->outside_mem = false;
        ++new_ptr;
        if (outside_mem){
            dynarr_info(new_ptr)->realloc_fn = dynarr_realloc_fn(ptr);
            memcpy(new_ptr, ptr, new_ptr->num*item_size);
        }
        return new_ptr;
    }
    else {
        // return the old pointer if we don't get memory
        dynarr_set_err(ptr, ds_alloc_fail);
        return ptr;
    }
}

// absorb the pointer normally emitted by reallocing
void _dynarr_free(void * ptr){
    if (ptr != NULL){
        realloc_fn_t realloc_fn = dynarr_realloc_fn(ptr);
        (void)realloc_fn(dynarr_info(ptr), 0);
    }
}
#define dynarr_free(ptr) _dynarr_free((ptr)); (ptr)=NULL

#define dynarr_set_cap(ptr, new_cap) (ptr) = bare_dynarr_realloc((ptr), (new_cap), sizeof(*(ptr)));

#define dynarr_set_len(ptr, new_len) \
    do {\
        if (new_len > dynarr_cap(ptr)){\
            dynarr_set_cap(ptr, new_len);\
            if (dynarr_cap(ptr) >= new_len){\
                dynarr_info(ptr)->num = new_len;\
                dynarr_set_err(ptr, ds_success);\
            } \
        } else { \
            dynarr_info(ptr)->num = new_len; \
            dynarr_set_err(ptr, ds_success);\
        }\
    }while (0)

void* bare_dynarr_maybe_grow(void * ptr, uintptr_t new_count, uintptr_t item_size){
    uintptr_t cap = dynarr_cap(ptr);
    if (cap < new_count){
        // use a growth factor of around 1.5
        while (cap < new_count){
            cap += (cap >> 1) + 8;
        }
        return bare_dynarr_realloc(ptr, cap, item_size);
    }
    else {
        dynarr_set_err(ptr, ds_success);
        return ptr;
    }
}

#define dynarr_maybe_grow(ptr, new_count) (ptr) = bare_dynarr_maybe_grow((ptr), (new_count), sizeof(*(ptr)))

void bare_dyarr_deln(void* ptr, uintptr_t del_i, uintptr_t n, uintptr_t item_size){
    if (del_i + n <= dynarr_num(ptr)){
        uint8_t *start_ptr = (uint8_t*)(ptr  + item_size*del_i);
        uint8_t *end_ptr = start_ptr + item_size*n;
        memmove(start_ptr, end_ptr, item_size*(dynarr_num(ptr) - del_i - n));
        dynarr_info(ptr)->num -= n;
        dynarr_set_err(ptr, ds_success);
    }
    else {
        dynarr_set_err(ptr, ds_out_of_bounds);
    }
}

#define dynarr_deln(ptr, del_i, n) bare_dyarr_deln(ptr, del_i, n, sizeof(*ptr))

#define dynarr_del(ptr, del_i) dynarr_deln(ptr, del_i, 1)

#define dynarr_pop(ptr) dynarr_del(ptr, dynarr_num(ptr) - 1)

void bare_dyarr_insertn(void* ptr, void* items, uintptr_t start_i, uintptr_t n, uintptr_t item_size){
    if (dynarr_cap(ptr) >= dynarr_num(ptr) + n && start_i <= dynarr_num(ptr)){
        uint8_t *cpy_start = ((uint8_t*)ptr) + start_i*item_size;
        uint8_t * move_end = cpy_start + n*item_size;
        memmove(move_end, cpy_start, (dynarr_num(ptr) - start_i)*item_size );
        memcpy(cpy_start, items, n*item_size);
        dynarr_info(ptr)->num += n;
        dynarr_set_err(ptr, ds_success);
    }
    else if (start_i >= dynarr_num(ptr) && dynarr_err(ptr) != ds_alloc_fail) {
        dynarr_set_err(ptr, ds_out_of_bounds);
    }
}
#define dynarr_insertn(ptr, items, start_i, n)\
    do{\
        dynarr_maybe_grow(ptr, dynarr_num(ptr) + n);\
        bare_dyarr_insertn(ptr, items, start_i, n, sizeof(*ptr));\
    }while(0)

// I could make this a function, but then it would not be able to take integer literals
#define dynarr_insert(ptr, item, start_i)\
    do{\
        dynarr_maybe_grow(ptr, dynarr_num(ptr) + 1);\
        if (dynarr_cap(ptr) >= dynarr_num(ptr) + 1 && start_i <= dynarr_num(ptr)){\
            memmove(&ptr[start_i + 1], &ptr[start_i], sizeof(*ptr)*(dynarr_num(ptr) - start_i));\
            ptr[start_i] = item;\
            ++dynarr_info(ptr)->num;\
            dynarr_set_err(ptr, ds_success);\
        }else if (start_i >= dynarr_num(ptr) && dynarr_err(ptr) != ds_alloc_fail){\
            dynarr_set_err(ptr, ds_out_of_bounds);\
        }\
    }while(0)

#define dynarr_appendn(ptr, items, n) dynarr_insertn(ptr, items, dynarr_num(ptr), n)

#define dynarr_append(ptr, item) dynarr_insert(ptr, item, dynarr_num(ptr))
