#include "c_dynarr_hm.h"
#include <stdio.h>
#include <assert.h>


#define ERROR_HERE printf("ERROR! %u\n", __LINE__);
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
    } while(0);

int main(){

    uint8_t *ptr = NULL;
    TEST("NULL get_dynarr_cap()", get_dynarr_cap(ptr) == 0);
    TEST("NULL get_dynarr_len()", get_dynarr_len(ptr) == 0);

    ptr = dynarr_alloc(uint8_t, 7);
    TEST("dynarr_alloc()", get_dynarr_cap(ptr) == 7);

    dynarr_set_cap(ptr, 17);
    TEST("dynarr_set_cap()", get_dynarr_cap(ptr) == 17);

    dynarr_set_len(ptr, 30);
    TEST("dynarr_set_len()", get_dynarr_len(ptr) == 30);
    TEST("dynarr_set_len()", get_dynarr_cap(ptr) == 30);

    dynarr_set_len(ptr, 0);
    TEST("dynarr_set_len()", get_dynarr_len(ptr) == 0);
    TEST("dynarr_set_len()", get_dynarr_cap(ptr) == 30);
 
    dynarr_maybe_grow(ptr, 32);
    TEST("dynarr_maybe_grow()", get_dynarr_cap(ptr) >= 32);

    uintptr_t old_cap = get_dynarr_cap(ptr);
    dynarr_maybe_grow(ptr, 32);
    TEST("dynarr_maybe_grow()", old_cap == get_dynarr_cap(ptr));

    dynarr_append(ptr, 72);
    TEST("dynarr_append()", ptr[get_dynarr_len(ptr) - 1] == 72);
    TEST("dynarr_append()", get_dynarr_len(ptr) == 1);

    uintptr_t len = get_dynarr_len(ptr);
    dynarr_print(ptr);
    dynarr_pop(ptr);
    TEST("dynarr_pop()", get_dynarr_len(ptr) == len -1);

    len = get_dynarr_len(ptr);
    dynarr_append(ptr, 64);
    TEST("dynarr_append()", ptr[get_dynarr_len(ptr) - 1] == 64);
    TEST("dynarr_append()", get_dynarr_len(ptr) == len + 1);

    uint8_t vals[] = { 3,6,7,2,3,3};
    len = get_dynarr_len(ptr);
    dynarr_appendn(ptr, vals, sizeof(vals));
    TEST("dynarr_appendn()", memcmp(&ptr[len], vals, sizeof(vals)) == 0);
    TEST("dynarr_appendn()", len + sizeof(vals) == get_dynarr_len(ptr));

    len = get_dynarr_len(ptr);
    uint8_t t_val = ptr[1];
    dynarr_del(ptr, 1);
    TEST("dynarr_del()", ptr[1] != t_val);
    TEST("dynarr_del()", get_dynarr_len(ptr) == len - 1);

    len = get_dynarr_len(ptr);
    dynarr_deln(ptr, len - 4, 3);
    TEST("dynarr_deln()", get_dynarr_len(ptr) == len - 3);

    dynarr_print(ptr);
    uintptr_t start_i = get_dynarr_len(ptr) - 2;
    len = get_dynarr_len(ptr);
    dynarr_insertn(ptr, vals, start_i, sizeof(vals) );
    TEST("dynarr_insertn()", memcmp(&ptr[start_i], vals, sizeof(vals)) == 0);
    TEST("dynarr_insertn()", get_dynarr_len(ptr) == len + sizeof(vals));

    uintptr_t insert_i = get_dynarr_len(ptr) - 3;
    len = get_dynarr_len(ptr);
    dynarr_insert(ptr, 8, insert_i);
    TEST("dynarr_insert()", ptr[insert_i] == 8);
    TEST("dynarr_insert()", get_dynarr_len(ptr) == len + 1);

    dynarr_free(ptr);

    return 0;
}
