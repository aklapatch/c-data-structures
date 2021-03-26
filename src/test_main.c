#include "dynarr.h"
#include <assert.h>
#include "test_helpers.h"

void print_vals(uint8_t * ptr){
    for (int i = 0; i < dynarr_len(ptr); ++i){
        printf("%d=%u\n",i, ptr[i]);
    }
}

// I don't have a reliable way of testing allocation failure. I would
// have to overlay the pointer to a temporary buffer or something, and
// then set the allocator to be a bad function that fails. (tries to
// explain why he can't do something and then comes up with a way to do
// it in the next sentence)
int main(){

    uint8_t *ptr = NULL;
    TEST("NULL dynarr_cap()", dynarr_cap(ptr) == 0);
    TEST("NULL dynarr_len()", dynarr_len(ptr) == 0);
    TESTERRFAIL("NULL error check", ptr, ds_null_ptr);

    // try saying the buffer is too small to check error handling
    dynarr_info other_buf[2];
    ptr = dynarr_init_from_buf(uint8_t, other_buf, sizeof(dynarr_info) - 1);
    TEST("init from buf failure", ptr == NULL);
    
    // test weird alignment
    uint32_t buf32[sizeof(dynarr_info)];
    uint32_t* buf_ptr = ((uintptr_t)buf32 % sizeof(uintptr_t) == 0) ? buf32 + 1 : buf32;
    ptr = dynarr_init_from_buf(uint8_t, buf_ptr, sizeof(buf32) - 4);
    TEST("init from buf alignment", (uint32_t*)ptr != buf_ptr);

    ptr = dynarr_init_from_buf(uint8_t, other_buf, sizeof(other_buf));
    TEST("init from buf ", ptr != NULL);
    TEST("init from buf ", dynarr_len(ptr) == 0);
    TEST("init from buf ", dynarr_cap(ptr) == sizeof(dynarr_info));
    TEST("init from buf ", dynarr_outside_mem(ptr) == true);
    TESTERRSUCCESS("init from buf", ptr);

    // Append a couple items
    uint8_t items[] = { 3,4,5};
    dynarr_appendn(ptr, items, sizeof(items));
    TEST("Appendn init from buf", dynarr_len(ptr) == sizeof(items));
    TEST("appendn init from buf contents", memcmp(ptr, items, sizeof(items)) == 0);
    TESTERRSUCCESS("appendn from buf contents", ptr);

    uint8_t * old_ptr = ptr;
    dynarr_set_cap(ptr, sizeof(other_buf));
    TEST("Resize from init_from_buf", ptr != old_ptr);
    TEST("Resize from init_from_buf", dynarr_outside_mem(ptr) == false);
    TESTERRSUCCESS("Resize from init from buf", ptr);
    dynarr_free(ptr);

    ptr = dynarr_alloc(uint8_t, 7);
    TEST("dynarr_alloc()", dynarr_cap(ptr) == 7);
    TEST("dynarr_alloc() memset", memset(ptr, 0, dynarr_cap(ptr)));
    TEST("dynarr_alloc() len", dynarr_len(ptr) == 0);
    TESTERRSUCCESS("dynarr_alloc()", ptr);

    dynarr_set_cap(ptr, 17);
    TEST("dynarr_set_cap()", dynarr_cap(ptr) == 17);
    TEST("dynarr_set_cap() memset", memset(ptr, 0, dynarr_cap(ptr)));
    TESTERRSUCCESS("dynarr_set_cap()", ptr);

    dynarr_set_len(ptr, 30);
    TEST("dynarr_set_len()", dynarr_len(ptr) == 30);
    TEST("dynarr_set_len()", dynarr_cap(ptr) == 30);
    TEST("dynarr_set_len() memset", memset(ptr, 0, dynarr_len(ptr)));
    TESTERRSUCCESS("dynarr_set_len()", ptr);

    dynarr_set_len(ptr, 0);
    TEST("dynarr_set_len()", dynarr_len(ptr) == 0);
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
    TEST("dynarr_append()", ptr[dynarr_len(ptr) - 1] == 72);
    TEST("dynarr_append()", dynarr_len(ptr) == 1);
    TESTERRSUCCESS("dynarr_append()", ptr);

    uintptr_t pre_pop_len = dynarr_len(ptr);
    dynarr_pop(ptr);
    TEST("dynarr_pop()", dynarr_len(ptr) == pre_pop_len -1);
    TESTERRSUCCESS("dynarr_pop()", ptr);

    uintptr_t pre_append_len = dynarr_len(ptr);
    dynarr_append(ptr, 64);
    TEST("dynarr_append()", ptr[pre_append_len] == 64);
    TEST("dynarr_append()", (dynarr_len(ptr) == (pre_append_len + 1)));
    TESTERRSUCCESS("dynarr_append()", ptr);

    uint8_t vals[] = { 3,6,7,2,3,3};
    uint8_t appendn_old_val = ptr[pre_append_len];
    uintptr_t pre_appendn_len = dynarr_len(ptr);
    dynarr_appendn(ptr, vals, sizeof(vals));
    print_vals(ptr);
    TEST("dynarr_appendn()", memcmp(&ptr[pre_appendn_len], vals, sizeof(vals)) == 0);
    TEST("dynarr_appendn()", pre_appendn_len + sizeof(vals) == dynarr_len(ptr));
    TEST("dynarr_appendn() old val preserved", ptr[pre_append_len] == appendn_old_val);
    TESTERRSUCCESS("dynarr_appendn()", ptr);

    uintptr_t len = dynarr_len(ptr);
    uint8_t first_val = ptr[0];
    uint8_t later_val = ptr[2];
    uint8_t t_val = ptr[1];
    dynarr_del(ptr, 1);
    TEST("dynarr_del()", ptr[1] != t_val);
    TEST("dynarr_del()", ptr[0] == first_val);
    TEST("dynarr_del()", ptr[1] == later_val);
    TEST("dynarr_del()", dynarr_len(ptr) == len - 1);
    TESTERRSUCCESS("dynarr_del()", ptr);

    len = dynarr_len(ptr);
    uintptr_t marker_i = len- 4;
    uint8_t pre_marker_val = ptr[marker_i - 1];
    uintptr_t del_len = 3;
    uint8_t post_del_val = ptr[marker_i + del_len + 1];
    dynarr_deln(ptr, marker_i, del_len);
    TEST("dynarr_deln()", dynarr_len(ptr) == len - del_len);
    TEST("dynarr_deln()", pre_marker_val == ptr[marker_i - 1]);
    TEST("dynarr_deln()", post_del_val == ptr[marker_i]);
    TESTERRSUCCESS("dynarr_deln()", ptr);

    uintptr_t start_i = dynarr_len(ptr) - 2;
    len = dynarr_len(ptr);
    uint8_t pre_insert_val = ptr[start_i];
    dynarr_insertn(ptr, vals, start_i, sizeof(vals) );
    TEST("dynarr_insertn()", memcmp(&ptr[start_i], vals, sizeof(vals)) == 0);
    TEST("dynarr_insertn()", dynarr_len(ptr) == len + sizeof(vals));
    TEST("dynarr_insertn()", ptr[start_i + sizeof(vals)] == pre_insert_val);
    TESTERRSUCCESS("dynarr_insertn()", ptr);

    uintptr_t insert_i = dynarr_len(ptr) - 3;
    len = dynarr_len(ptr);
    uint8_t other_val = ptr[insert_i];
    uint8_t pre_val = ptr[insert_i-1];
    dynarr_insert(ptr, 8, insert_i);
    TEST("dynarr_insert()", ptr[insert_i] == 8);
    TEST("dynarr_insert()", dynarr_len(ptr) == len + 1);
    TEST("dynarr_insert()", ptr[insert_i+1] == other_val);
    TEST("dynarr_insert()", ptr[insert_i-1] == pre_val);
    TESTERRSUCCESS("dynarr_insert()", ptr);

    // check out of bounds errors
    dynarr_insertn(ptr, vals, dynarr_len(ptr)+1, sizeof(vals));
    TESTERRFAIL("dynarr_insertn() oob", ptr, ds_out_of_bounds); 

    dynarr_insert(ptr, 1, dynarr_len(ptr)+1);
    TESTERRFAIL("dynarr_insert() oob", ptr, ds_out_of_bounds); 

    dynarr_deln(ptr, dynarr_len(ptr)+1, 3);
    TESTERRFAIL("dynarr_deln() oob", ptr, ds_out_of_bounds); 

    dynarr_del(ptr, dynarr_len(ptr) + 1);
    TESTERRFAIL("dynarr_del() oob", ptr, ds_out_of_bounds); 

    dynarr_free(ptr);

    return 0;
}
