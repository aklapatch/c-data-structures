#include "dynarr.h"
#include <stdio.h>
#include <assert.h>

// I need to do something to handle errors
// I can return NULL from the realloc functions , but then the user has to catch the error, and I can't think of a way to do that whithout making temp variables that pollute the name space.
// I could say that you have to make a new ptr to get stuff, but then it will lose memory if the previous function fails.
// STB does not handle allocation failure.
//
// The macro can set the passed in ptr to the return value only if it is not NULL.
// Then the user will need to check if the capacity changed.
// We may still be able to return an error code though.
//
//
void print_dynarr(void * ptr){
    uintptr_t len = get_dynarr_len(ptr);
    uint8_t * data_ptr = ptr;
    for(int i = 0; i < len; ++i){
        printf("item: %d = %u\n", i, data_ptr[i]);
    }
}

#define TEST(label, expression)\
    do{\
        printf("Test %s\n", label);\
        printf("Testing: (%s)", #expression);\
        assert(expression);\
        printf(": ok\n\n");\
    } while(0)

#define TESTERRSUCCESS(message, ptr)\
    do{\
        TEST(message " err val check", get_dynarr_err(ptr) == ds_success);\
        TEST(message " err set check", !dynarr_is_err_set(ptr));\
    }while (0)

#define TESTERRFAIL(message, ptr, err_val)\
    do{\
        TEST(message " err val check", get_dynarr_err(ptr) == err_val);\
        TEST(message " err set check", dynarr_is_err_set(ptr));\
    }while (0)

int main(){

    uint8_t *ptr = NULL;
    TEST("NULL get_dynarr_cap()", get_dynarr_cap(ptr) == 0);
    TEST("NULL get_dynarr_len()", get_dynarr_len(ptr) == 0);

    ptr = dynarr_alloc(uint8_t, 7);
    TEST("dynarr_alloc()", get_dynarr_cap(ptr) == 7);
    TEST("dynarr_alloc() memset", memset(ptr, 0, get_dynarr_cap(ptr)));
    TEST("dynarr_alloc() len", get_dynarr_len(ptr) == 0);
    TESTERRSUCCESS("dynarr_alloc()", ptr);

    dynarr_set_cap(ptr, 17);
    TEST("dynarr_set_cap()", get_dynarr_cap(ptr) == 17);
    TEST("dynarr_set_cap() memset", memset(ptr, 0, get_dynarr_cap(ptr)));
    TESTERRSUCCESS("dynarr_set_cap()", ptr);

    dynarr_set_len(ptr, 30);
    TEST("dynarr_set_len()", get_dynarr_len(ptr) == 30);
    TEST("dynarr_set_len()", get_dynarr_cap(ptr) == 30);
    TEST("dynarr_set_len() memset", memset(ptr, 0, get_dynarr_len(ptr)));
    TESTERRSUCCESS("dynarr_set_len()", ptr);

    dynarr_set_len(ptr, 0);
    TEST("dynarr_set_len()", get_dynarr_len(ptr) == 0);
    TEST("dynarr_set_len()", get_dynarr_cap(ptr) == 30);
    TESTERRSUCCESS("dynarr_set_len()", ptr);
 
    dynarr_maybe_grow(ptr, 32);
    TEST("dynarr_maybe_grow()", get_dynarr_cap(ptr) >= 32);
    TESTERRSUCCESS("dynarr_maybe_grow()", ptr);

    uintptr_t old_cap = get_dynarr_cap(ptr);
    dynarr_maybe_grow(ptr, 32);
    TEST("dynarr_maybe_grow()", old_cap == get_dynarr_cap(ptr));
    TESTERRSUCCESS("dynarr_maybe_grow()", ptr);

    dynarr_append(ptr, 72);
    TEST("dynarr_append()", ptr[get_dynarr_len(ptr) - 1] == 72);
    TEST("dynarr_append()", get_dynarr_len(ptr) == 1);
    TESTERRSUCCESS("dynarr_append()", ptr);

    uintptr_t pre_pop_len = get_dynarr_len(ptr);
    dynarr_pop(ptr);
    TEST("dynarr_pop()", get_dynarr_len(ptr) == pre_pop_len -1);
    TESTERRSUCCESS("dynarr_pop()", ptr);

    uintptr_t pre_append_len = get_dynarr_len(ptr);
    dynarr_append(ptr, 64);
    TEST("dynarr_append()", ptr[pre_append_len] == 64);
    TEST("dynarr_append()", get_dynarr_len(ptr) == pre_append_len + 1);
    TESTERRSUCCESS("dynarr_append()", ptr);

    uint8_t vals[] = { 3,6,7,2,3,3};
    uintptr_t pre_appendn_len = get_dynarr_len(ptr);
    dynarr_appendn(ptr, vals, sizeof(vals));
    TEST("dynarr_appendn()", memcmp(&ptr[pre_appendn_len], vals, sizeof(vals)) == 0);
    TEST("dynarr_appendn()", pre_appendn_len + sizeof(vals) == get_dynarr_len(ptr));
    TEST("dynarr_appendn() old val preserved", ptr[pre_append_len] == 64);
    TESTERRSUCCESS("dynarr_appendn()", ptr);

    uintptr_t len = get_dynarr_len(ptr);
    uint8_t first_val = ptr[0];
    uint8_t later_val = ptr[2];
    uint8_t t_val = ptr[1];
    dynarr_del(ptr, 1);
    TEST("dynarr_del()", ptr[1] != t_val);
    TEST("dynarr_del()", ptr[0] == first_val);
    TEST("dynarr_del()", ptr[1] == later_val);
    TEST("dynarr_del()", get_dynarr_len(ptr) == len - 1);
    TESTERRSUCCESS("dynarr_del()", ptr);

    len = get_dynarr_len(ptr);
    uintptr_t marker_i = len- 4;
    uint8_t pre_marker_val = ptr[marker_i - 1];
    uintptr_t del_len = 3;
    uint8_t post_del_val = ptr[marker_i + del_len + 1];
    dynarr_deln(ptr, marker_i, del_len);
    TEST("dynarr_deln()", get_dynarr_len(ptr) == len - del_len);
    TEST("dynarr_deln()", pre_marker_val == ptr[marker_i - 1]);
    TEST("dynarr_deln()", post_del_val == ptr[marker_i]);
    TESTERRSUCCESS("dynarr_deln()", ptr);

    uintptr_t start_i = get_dynarr_len(ptr) - 2;
    len = get_dynarr_len(ptr);
    uint8_t pre_insert_val = ptr[start_i];
    dynarr_insertn(ptr, vals, start_i, sizeof(vals) );
    TEST("dynarr_insertn()", memcmp(&ptr[start_i], vals, sizeof(vals)) == 0);
    TEST("dynarr_insertn()", get_dynarr_len(ptr) == len + sizeof(vals));
    TEST("dynarr_insertn()", ptr[start_i + sizeof(vals)] == pre_insert_val);
    TESTERRSUCCESS("dynarr_insertn()", ptr);

    uintptr_t insert_i = get_dynarr_len(ptr) - 3;
    len = get_dynarr_len(ptr);
    uint8_t other_val = ptr[insert_i];
    uint8_t pre_val = ptr[insert_i-1];
    dynarr_insert(ptr, 8, insert_i);
    TEST("dynarr_insert()", ptr[insert_i] == 8);
    TEST("dynarr_insert()", get_dynarr_len(ptr) == len + 1);
    TEST("dynarr_insert()", ptr[insert_i+1] == other_val);
    TEST("dynarr_insert()", ptr[insert_i-1] == pre_val);
    TESTERRSUCCESS("dynarr_insert()", ptr);

    // check out of bounds errors
    dynarr_insertn(ptr, vals, get_dynarr_len(ptr)+1, sizeof(vals));
    TESTERRFAIL("dynarr_insertn() oob", ptr, ds_out_of_bounds); 

    dynarr_insert(ptr, 1, get_dynarr_len(ptr)+1);
    TESTERRFAIL("dynarr_insert() oob", ptr, ds_out_of_bounds); 

    dynarr_deln(ptr, get_dynarr_len(ptr)+1, 3);
    TESTERRFAIL("dynarr_deln() oob", ptr, ds_out_of_bounds); 

    dynarr_del(ptr, get_dynarr_len(ptr) + 1);
    TESTERRFAIL("dynarr_del() oob", ptr, ds_out_of_bounds); 

    dynarr_free(ptr);

    return 0;
}
