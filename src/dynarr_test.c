#include "dynarr.h"
#include <assert.h>
#include "test_helpers.h"
#include <stdlib.h>

void print_vals(uint8_t * ptr){
    for (uint32_t i = 0; i < dynarr_num(ptr); ++i){
        printf("%d=%u\n",i, ptr[i]);
    }
}

// define a function that just returns NULL to simulate allocation failure
void *bad_realloc(void*ptr, size_t size){
    (void)ptr, (void)size;
    return NULL;
}

// I don't have a reliable way of testing allocation failure. I would
// have to overlay the pointer to a temporary buffer or something, and
// then set the allocator to be a bad function that fails. (tries to
// explain why he can't do something and then comes up with a way to do
// it in the next sentence)
int main(){

    uint8_t *ptr = NULL;
    TESTGROUP("Init");
    TESTINTEQ(dynarr_cap(ptr), 0);
    TESTINTEQ(1, 0);
    TESTINTEQ(dynarr_num(ptr), 0);
    TESTINTEQ(dynarr_err(ptr), ds_null_ptr);
    
    // try saying the buffer is too small to check error handling
    TESTGROUP("buf init tests");
    uint8_t other_buf[48];
    dynarr_init_from_buf(ptr, other_buf, sizeof(dynarr_inf) - 1, realloc);
    TESTPTREQ(ptr, NULL);
    
    // test weird alignment
    uint8_t buf8[2*sizeof(dynarr_inf)];
    for (uint8_t* buf_ptr = buf8, i = 0; i < 8; ++i,++buf_ptr){
        dynarr_init_from_buf(ptr, buf_ptr, sizeof(buf8), realloc);
        TESTINTEQ((uintptr_t)ptr % sizeof(uintptr_t), 0);
    }

    dynarr_inf init_test_buf[2];
    dynarr_init_from_buf(ptr, init_test_buf, sizeof(init_test_buf), realloc);
    TESTPTRNEQ(ptr, NULL);
    TESTINTEQ(dynarr_num(ptr), 0);
    TESTINTEQ(dynarr_cap(ptr), sizeof(dynarr_inf));
    TESTINTEQ(dynarr_outside_mem(ptr), true);
    TESTINTEQ(dynarr_err(ptr), ds_success);

    TESTGROUP("append");
    // Append a couple items
    uint8_t items[] = { 3,4,5};
    dynarr_appendn(ptr, items, sizeof(items));
    TESTINTEQ(dynarr_num(ptr), sizeof(items));
    TESTINTEQ(memcmp(ptr, items, sizeof(items)), 0);
    TESTINTEQ(dynarr_err(ptr), ds_success);

    TESTGROUP("set cap");
    uint8_t * old_ptr = ptr;
    dynarr_set_cap(ptr, sizeof(other_buf));
    TESTPTRNEQ(ptr, old_ptr);
    TESTINTEQ(dynarr_num(ptr), sizeof(items));
    TESTINTEQ(dynarr_outside_mem(ptr), false);
    TESTINTEQ(dynarr_err(ptr), ds_success);

    dynarr_free(ptr);

    TESTGROUP("Init");
    dynarr_init(ptr, 7, realloc);
    TESTINTEQ(dynarr_cap(ptr), 7);
    memset(ptr, 0, dynarr_cap(ptr));
    TESTINTEQ(dynarr_num(ptr), 0);
    TESTINTEQ(dynarr_err(ptr), ds_success);

    TESTGROUP("set cap");
    dynarr_set_cap(ptr, 17);
    TESTINTEQ(dynarr_cap(ptr), 17);
    memset(ptr, 0, dynarr_cap(ptr));
    TESTINTEQ(dynarr_err(ptr), ds_success);

    TESTGROUP("set len");
    dynarr_set_len(ptr, 30);
    TESTINTEQ(dynarr_num(ptr), 30);
    TESTINTEQ(dynarr_cap(ptr), 30);
    memset(ptr, 0, dynarr_num(ptr));
    TESTINTEQ(dynarr_err(ptr), ds_success);

    dynarr_set_len(ptr, 0);
    TESTINTEQ(dynarr_num(ptr), 0);
    TESTINTEQ(dynarr_cap(ptr), 30);
    TESTINTEQ(dynarr_err(ptr), ds_success);
 
    TESTGROUP("maybe grow");
    dynarr_maybe_grow(ptr, 32);
    TESTINTEQ(dynarr_cap(ptr) >= 32, true);
    TESTINTEQ(dynarr_err(ptr), ds_success);

    uintptr_t old_cap = dynarr_cap(ptr);
    dynarr_maybe_grow(ptr, 32);
    TESTINTEQ(old_cap, dynarr_cap(ptr));
    TESTINTEQ(dynarr_err(ptr), ds_success);

    TESTGROUP("append");
    dynarr_append(ptr, 72);
    TESTINTEQ(ptr[dynarr_num(ptr)-1], 72);
    TESTINTEQ(dynarr_num(ptr), 1);
    TESTINTEQ(dynarr_err(ptr), ds_success);

    TESTGROUP("pop");
    uintptr_t pre_pop_len = dynarr_num(ptr);
    dynarr_pop(ptr);
    TESTINTEQ(dynarr_err(ptr), ds_success);
    TESTINTEQ(dynarr_num(ptr), pre_pop_len - 1);

    TESTGROUP("append");
    uintptr_t pre_append_len = dynarr_num(ptr);
    dynarr_append(ptr, 64);
    TESTINTEQ(ptr[pre_append_len], 64);
    TESTINTEQ(dynarr_num(ptr), pre_append_len + 1);
    TESTINTEQ(dynarr_err(ptr), ds_success);

    TESTGROUP("appendn");
    uint8_t vals[] = { 3,6,7,2,3,3};
    uint8_t appendn_old_val = ptr[pre_append_len];
    uintptr_t pre_appendn_len = dynarr_num(ptr);
    dynarr_appendn(ptr, vals, sizeof(vals));
    TESTINTEQ(dynarr_err(ptr), ds_success);
    TESTINTEQ(memcmp(&ptr[pre_appendn_len], vals, sizeof(vals)), 0);
    TESTINTEQ(pre_appendn_len + sizeof(vals), dynarr_num(ptr));
    TESTINTEQ(ptr[pre_append_len], appendn_old_val);

    TESTGROUP("del");
    uintptr_t len = dynarr_num(ptr);
    uint8_t first_val = ptr[0];
    uint8_t later_val = ptr[2];
    uint8_t t_val = ptr[1];
    dynarr_del(ptr, 1);
    TESTINTNEQ(ptr[1], t_val);
    TESTINTEQ(ptr[1], later_val);
    TESTINTEQ(ptr[0], first_val);
    TESTINTEQ(dynarr_num(ptr), len-1);
    TESTINTEQ(dynarr_err(ptr), ds_success);

    TESTGROUP("deln");
    len = dynarr_num(ptr);
    uintptr_t marker_i = len- 4;
    uint8_t pre_marker_val = ptr[marker_i - 1];
    uintptr_t del_len = 3;
    uint8_t post_del_val = ptr[marker_i + del_len + 1];
    dynarr_deln(ptr, marker_i, del_len);
    TESTINTEQ(dynarr_num(ptr), len-del_len);
    TESTINTEQ(pre_marker_val , ptr[marker_i - 1]);
    TESTINTEQ(post_del_val, ptr[marker_i]);
    TESTINTEQ(dynarr_err(ptr), ds_success);

    TESTGROUP("insertn");
    uintptr_t start_i = dynarr_num(ptr) - 2;
    len = dynarr_num(ptr);
    uint8_t pre_insert_val = ptr[start_i];
    dynarr_insertn(ptr, vals, start_i, sizeof(vals) );
    TESTINTEQ(memcmp(&ptr[start_i], vals, sizeof(vals)), 0);
    TESTINTEQ(dynarr_num(ptr), len + sizeof(vals));
    TESTINTEQ(ptr[start_i + sizeof(vals)], pre_insert_val);
    TESTINTEQ(dynarr_err(ptr), ds_success);

    TESTGROUP("insert");
    uintptr_t insert_i = dynarr_num(ptr) - 3;
    len = dynarr_num(ptr);
    uint8_t other_val = ptr[insert_i];
    uint8_t pre_val = ptr[insert_i-1];
    dynarr_insert(ptr, 8, insert_i);
    TESTINTEQ(ptr[insert_i] , 8);
    TESTINTEQ(dynarr_num(ptr), len + 1);
    TESTINTEQ(ptr[insert_i+1], other_val);;
    TESTINTEQ(ptr[insert_i-1], pre_val);
    TESTINTEQ(dynarr_err(ptr), ds_success);

    TESTGROUP("Out of bounds errors");
    // check out of bounds errors
    dynarr_insertn(ptr, vals, dynarr_num(ptr)+1, sizeof(vals));
    TESTINTEQ(dynarr_err(ptr), ds_out_of_bounds);

    dynarr_insert(ptr, 1, dynarr_num(ptr)+1);
    TESTINTEQ(dynarr_err(ptr), ds_out_of_bounds);

    dynarr_deln(ptr, dynarr_num(ptr)+1, 3);
    TESTINTEQ(dynarr_err(ptr), ds_out_of_bounds);

    dynarr_del(ptr, dynarr_num(ptr) + 1);
    TESTINTEQ(dynarr_err(ptr), ds_out_of_bounds);

    TESTGROUP("Free");
    dynarr_free(ptr);
    TESTPTREQ(ptr, NULL);

    // alloc fail tests!
    // --------------------------------------------------------------------
    TESTGROUP("Alloc fail");
    dynarr_init(ptr, 7, bad_realloc);
    TESTPTREQ(ptr, NULL);

    // overlay the pointer over a struct so that it is not null, but has
    // memory to hold errors
    dynarr_inf buf[1] = {0};
    dynarr_init_from_buf(ptr, buf, sizeof(buf), bad_realloc);

    uintptr_t too_big = 2*sizeof(buf);

    dynarr_set_cap(ptr, too_big);
    TESTINTEQ(dynarr_err(ptr), ds_alloc_fail);

    dynarr_set_len(ptr, too_big);
    TESTINTEQ(dynarr_err(ptr), ds_alloc_fail);

    dynarr_maybe_grow(ptr, too_big);
    TESTINTEQ(dynarr_err(ptr), ds_alloc_fail);

    dynarr_append(ptr, 72);
    TESTINTEQ(dynarr_err(ptr), ds_alloc_fail);

    dynarr_appendn(ptr, vals, sizeof(vals));
    TESTINTEQ(dynarr_err(ptr), ds_alloc_fail);

    dynarr_insertn(ptr, vals, 0, sizeof(vals) );
    TESTINTEQ(dynarr_err(ptr), ds_alloc_fail);

    dynarr_insert(ptr, 8, 0);
    TESTINTEQ(dynarr_err(ptr), ds_alloc_fail);

    return 0;
}
