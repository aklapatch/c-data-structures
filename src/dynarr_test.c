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

    // Append a couple items
    uint8_t items[] = { 3,4,5};
    dynarr_appendn(ptr, items, sizeof(items));
    TEST("Appendn init from buf", dynarr_num(ptr) == sizeof(items));
    TEST("appendn init from buf contents", memcmp(ptr, items, sizeof(items)) == 0);
    TESTERRSUCCESS("appendn from buf contents", ptr);

    uint8_t * old_ptr = ptr;
    dynarr_set_cap(ptr, sizeof(other_buf));
    TEST("Resize from init_from_buf", ptr != old_ptr);
    TEST("Appendn init from buf", dynarr_num(ptr) == sizeof(items));
    TEST("Resize from init_from_buf", dynarr_outside_mem(ptr) == false);
    TESTERRSUCCESS("Resize from init from buf", ptr);
    dynarr_free(ptr);

    dynarr_init(ptr, 7, realloc);
    TEST("dynarr_alloc()", dynarr_cap(ptr) == 7);
    TEST("dynarr_alloc() memset", memset(ptr, 0, dynarr_cap(ptr)));
    TEST("dynarr_alloc() len", dynarr_num(ptr) == 0);
    TESTERRSUCCESS("dynarr_alloc()", ptr);

    dynarr_set_cap(ptr, 17);
    TEST("dynarr_set_cap()", dynarr_cap(ptr) == 17);
    TEST("dynarr_set_cap() memset", memset(ptr, 0, dynarr_cap(ptr)));
    TESTERRSUCCESS("dynarr_set_cap()", ptr);

    dynarr_set_len(ptr, 30);
    TEST("dynarr_set_len()", dynarr_num(ptr) == 30);
    TEST("dynarr_set_len()", dynarr_cap(ptr) == 30);
    TEST("dynarr_set_len() memset", memset(ptr, 0, dynarr_num(ptr)));
    TESTERRSUCCESS("dynarr_set_len()", ptr);

    dynarr_set_len(ptr, 0);
    TEST("dynarr_set_len()", dynarr_num(ptr) == 0);
    TEST("dynarr_set_len()", dynarr_cap(ptr) == 30);
    TESTERRSUCCESS("dynarr_set_len()", ptr);
 
    dynarr_maybe_grow(ptr, 32);
    TEST("dynarr_maybe_grow()", dynarr_cap(ptr) >= 32);
    TESTERRSUCCESS("dynarr_maybe_grow()", ptr);

    uintptr_t old_cap = dynarr_cap(ptr);
    dynarr_maybe_grow(ptr, 32);
    TEST("dynarr_maybe_grow()", old_cap == dynarr_cap(ptr));
    TESTERRSUCCESS("dynarr_maybe_grow()", ptr);

    dynarr_append(ptr, 72);
    TEST("dynarr_append()", ptr[dynarr_num(ptr) - 1] == 72);
    TEST("dynarr_append()", dynarr_num(ptr) == 1);
    TESTERRSUCCESS("dynarr_append()", ptr);

    uintptr_t pre_pop_len = dynarr_num(ptr);
    dynarr_pop(ptr);
    TEST("dynarr_pop()", dynarr_num(ptr) == pre_pop_len -1);
    TESTERRSUCCESS("dynarr_pop()", ptr);

    uintptr_t pre_append_len = dynarr_num(ptr);
    dynarr_append(ptr, 64);
    TEST("dynarr_append()", ptr[pre_append_len] == 64);
    TEST("dynarr_append()", (dynarr_num(ptr) == (pre_append_len + 1)));
    TESTERRSUCCESS("dynarr_append()", ptr);

    uint8_t vals[] = { 3,6,7,2,3,3};
    uint8_t appendn_old_val = ptr[pre_append_len];
    uintptr_t pre_appendn_len = dynarr_num(ptr);
    dynarr_appendn(ptr, vals, sizeof(vals));
    TEST("dynarr_appendn()", memcmp(&ptr[pre_appendn_len], vals, sizeof(vals)) == 0);
    TEST("dynarr_appendn()", pre_appendn_len + sizeof(vals) == dynarr_num(ptr));
    TEST("dynarr_appendn() old val preserved", ptr[pre_append_len] == appendn_old_val);
    TESTERRSUCCESS("dynarr_appendn()", ptr);

    uintptr_t len = dynarr_num(ptr);
    uint8_t first_val = ptr[0];
    uint8_t later_val = ptr[2];
    uint8_t t_val = ptr[1];
    dynarr_del(ptr, 1);
    TEST("dynarr_del()", ptr[1] != t_val);
    TEST("dynarr_del()", ptr[0] == first_val);
    TEST("dynarr_del()", ptr[1] == later_val);
    TEST("dynarr_del()", dynarr_num(ptr) == len - 1);
    TESTERRSUCCESS("dynarr_del()", ptr);

    len = dynarr_num(ptr);
    uintptr_t marker_i = len- 4;
    uint8_t pre_marker_val = ptr[marker_i - 1];
    uintptr_t del_len = 3;
    uint8_t post_del_val = ptr[marker_i + del_len + 1];
    dynarr_deln(ptr, marker_i, del_len);
    TEST("dynarr_deln()", dynarr_num(ptr) == len - del_len);
    TEST("dynarr_deln()", pre_marker_val == ptr[marker_i - 1]);
    TEST("dynarr_deln()", post_del_val == ptr[marker_i]);
    TESTERRSUCCESS("dynarr_deln()", ptr);

    uintptr_t start_i = dynarr_num(ptr) - 2;
    len = dynarr_num(ptr);
    uint8_t pre_insert_val = ptr[start_i];
    dynarr_insertn(ptr, vals, start_i, sizeof(vals) );
    TEST("dynarr_insertn()", memcmp(&ptr[start_i], vals, sizeof(vals)) == 0);
    TEST("dynarr_insertn()", dynarr_num(ptr) == len + sizeof(vals));
    TEST("dynarr_insertn()", ptr[start_i + sizeof(vals)] == pre_insert_val);
    TESTERRSUCCESS("dynarr_insertn()", ptr);

    uintptr_t insert_i = dynarr_num(ptr) - 3;
    len = dynarr_num(ptr);
    uint8_t other_val = ptr[insert_i];
    uint8_t pre_val = ptr[insert_i-1];
    dynarr_insert(ptr, 8, insert_i);
    TEST("dynarr_insert()", ptr[insert_i] == 8);
    TEST("dynarr_insert()", dynarr_num(ptr) == len + 1);
    TEST("dynarr_insert()", ptr[insert_i+1] == other_val);
    TEST("dynarr_insert()", ptr[insert_i-1] == pre_val);
    TESTERRSUCCESS("dynarr_insert()", ptr);

    // check out of bounds errors
    dynarr_insertn(ptr, vals, dynarr_num(ptr)+1, sizeof(vals));
    TESTERRFAIL("dynarr_insertn() oob", ptr, ds_out_of_bounds); 

    dynarr_insert(ptr, 1, dynarr_num(ptr)+1);
    TESTERRFAIL("dynarr_insert() oob", ptr, ds_out_of_bounds); 

    dynarr_deln(ptr, dynarr_num(ptr)+1, 3);
    TESTERRFAIL("dynarr_deln() oob", ptr, ds_out_of_bounds); 

    dynarr_del(ptr, dynarr_num(ptr) + 1);
    TESTERRFAIL("dynarr_del() oob", ptr, ds_out_of_bounds); 

    dynarr_free(ptr);

    // alloc fail tests!
    // --------------------------------------------------------------------
    ptr = NULL;
    dynarr_init(ptr, 7, bad_realloc);
    TEST("dynarr_alloc() fail", ptr == NULL);

    // overlay the pointer over a struct so that it is not null, but has
    // memory to hold errors
    dynarr_inf buf[1] = {0};
    dynarr_init_from_buf(ptr, buf, sizeof(buf), bad_realloc);

    uintptr_t too_big = 2*sizeof(buf);

    dynarr_set_cap(ptr, too_big);
    TESTERRFAIL("dynarr_set_cap() alloc fail", ptr, ds_alloc_fail);

    dynarr_set_len(ptr, too_big);
    TESTERRFAIL("dynarr_set_len() alloc fail", ptr, ds_alloc_fail);

    dynarr_maybe_grow(ptr, too_big);
    TESTERRFAIL("dynarr_maybe_grow() alloc fail", ptr, ds_alloc_fail);

    dynarr_append(ptr, 72);
    TESTERRFAIL("dynarr_append() alloc fail", ptr, ds_alloc_fail);

    dynarr_appendn(ptr, vals, sizeof(vals));
    TESTERRFAIL("dynarr_appendn() alloc fail", ptr, ds_alloc_fail);

    dynarr_insertn(ptr, vals, 0, sizeof(vals) );
    TESTERRFAIL("dynarr_insertn() alloc fail", ptr, ds_alloc_fail);

    dynarr_insert(ptr, 8, 0);
    TESTERRFAIL("dynarr_insert() alloc fail", ptr, ds_alloc_fail);

    return 0;
}
