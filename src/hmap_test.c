#include"hmap.h"
#include "ahash.h"
#include "test_helpers.h"
#include <stdlib.h>

int main(){
    
    uint16_t *hmap = NULL;
    hm_init(hmap, 32, realloc, ahash_buf);

    TEST("hm_num @ init", hm_num(hmap) == 0);
    TEST("hm_cap @ init w 32", hm_cap(hmap) == 32);

    uint16_t test_val = 64;
    uint64_t key = 72;
    hm_set(hmap, key, test_val);
    TESTERRSUCCESS("hm_set()", hmap, true);

    uint16_t searched_val = 0;
    hm_get(hmap, key, searched_val);
    TESTERRSUCCESS("hm_get()", hmap, true);
    TEST("Search works for inserted val", searched_val == test_val);

    // insert a lot of values and see how this goes.
    uint16_t ins_vals[31];
    uint64_t keys[31];
    for (uint32_t i = 0; i < sizeof(ins_vals)/sizeof(ins_vals[0]); ++i){
        ins_vals[i] = i;
        keys[i] = i;
        // insert the value
        printf("i=%d key=%lu val=%u\n", i, keys[i], ins_vals[i]);
        hm_set(hmap, keys[i],ins_vals[i]);
        TESTERRSUCCESS("hm fill up", hmap, true);

        uint16_t out_val = UINT16_MAX;
        hm_get(hmap, keys[i], out_val);
        TESTERRSUCCESS("hm query", hmap, true);
        TEST("hm query result", out_val == ins_vals[i]);
    }
    // make sure all values didn't get overwritten
    for (uint32_t i = 0; i < sizeof(ins_vals)/sizeof(ins_vals[0]); ++i){
        uint16_t out_val = UINT16_MAX;
        hm_get(hmap, keys[i], out_val);
        printf("i=%d key=%lx val=%x\n", i, keys[i], out_val);
        TESTERRSUCCESS("hm query", hmap, true);
        TEST("hm query result", out_val == ins_vals[i]);
    }

    hm_realloc(hmap, 64);
    TESTERRSUCCESS("hm realloc", hmap, true);
    // try resizing the hmap and see if all the key are still there
    for (uint32_t i = 0; i < sizeof(ins_vals)/sizeof(ins_vals[0]); ++i){
        uint16_t out_val = UINT16_MAX;
        printf("key=%lx\n", keys[i]);
        hm_get(hmap, keys[i], out_val);
        TESTERRSUCCESS("hm realloc query", hmap, true);
        TEST("hm realloc query result", out_val == ins_vals[i]);
    }
    // try deleting all the keys and make sure they're gone
    for (uint32_t i = 0; i < sizeof(ins_vals)/sizeof(ins_vals[0]); ++i){
        // insert the value
        // removing key 3 causes a chain of moves that I'm still debuggin
        // removing 3 seems to work fine
        // 11 is on a list with 3 and 9, but should be at bucket 3 slot 2
        // it may be getting moved to a slot where it does not belong, which is weird
        // I need to implement direct hit tracking to fix this I think.
        hm_del(hmap, keys[i]);
        printf("i=%d key=%lx\n", i, keys[i]);
        TESTERRSUCCESS("hm delete", hmap, true);

        uint16_t out_val = UINT16_MAX;
        hm_get(hmap, keys[i], out_val);
        TESTERRFAIL("hm del query", hmap, ds_not_found, true);
    }

    hm_free(hmap);
    TEST("hmap free", hmap == NULL);

    hm_init(hmap, 32, realloc, ahash_buf);
    // insert a stupid number of keys and see if it still works
    for (uint32_t i = 0; i < UINT16_MAX; ++i){
        hm_set(hmap, i, i);
        TESTERRSUCCESS("hm bulk insert", hmap, true);
    }
    for (uint32_t i = 0; i < UINT16_MAX; ++i){
        uint16_t out_val = UINT16_MAX;
        printf("key=%u\n", i);
        hm_get(hmap, i, out_val);
        TESTERRSUCCESS("hm bulk get after insert round 2", hmap, true);
        TEST("hm get == set", i == out_val);
    }

    return 0;
}
