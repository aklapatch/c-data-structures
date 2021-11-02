#include<stddef.h>
// define a function that just returns NULL to simulate allocation failure
void *bad_realloc(void*ptr, size_t size){
    return NULL;
}

#include "dynarr.h"
#include <assert.h>
#include "test_helpers.h"

int main(){

    
    uint8_t *ptr = NULL;
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

    uint8_t vals[] = { 3,6,7,2,3,3};
    dynarr_appendn(ptr, vals, sizeof(vals));
    TESTERRFAIL("dynarr_appendn() alloc fail", ptr, ds_alloc_fail);

    dynarr_insertn(ptr, vals, 0, sizeof(vals) );
    TESTERRFAIL("dynarr_insertn() alloc fail", ptr, ds_alloc_fail);

    dynarr_insert(ptr, 8, 0);
    TESTERRFAIL("dynarr_insert() alloc fail", ptr, ds_alloc_fail);

    return 0;
}
