#include<stddef.h>
// define a function that just returns NULL to simulate allocation failure
void * bad_realloc(void*ptr, size_t size){
    return NULL;
}

#define C_DS_REALLOC bad_realloc
#include "dynarr.h"
#include <assert.h>
#include "test_helpers.h"

int main(){

    c_ds_info buf[2] = {0};
    
    uint8_t *ptr = dynarr_alloc(uint8_t, 7);
    TEST("dynarr_alloc() fail", ptr == NULL);

    // overlay the pointer over a struct so that it is not null, but has
    // memory to hold errors
    ptr = (uint8_t*)&buf[1];

    dynarr_set_cap(ptr, 17);
    TESTERRFAIL("dynarr_set_cap() alloc fail", ptr, ds_alloc_fail);

    dynarr_set_len(ptr, 30);
    TESTERRFAIL("dynarr_set_len() alloc fail", ptr, ds_alloc_fail);

    dynarr_maybe_grow(ptr, 32);
    TESTERRFAIL("dynarr_maybe_grow() alloc fail", ptr, ds_alloc_fail);

    dynarr_append(ptr, 72);
    TESTERRFAIL("dynarr_append() alloc fail", ptr, ds_alloc_fail);

    uint8_t vals[] = { 3,6,7,2,3,3};
    dynarr_appendn(ptr, vals, sizeof(vals));
    TESTERRFAIL("dynarr_appendn() alloc fail", ptr, ds_alloc_fail);

    dynarr_insertn(ptr, vals, 0, sizeof(vals) );
    TESTERRFAIL("dynarr_insertn() alloc fail", ptr, ds_alloc_fail);

    dynarr_insert(ptr, 8, 0);
    TESTERRFAIL("dynarr_insert() alloc fail", ptr, ds_alloc_fail);

    return 0;
}
