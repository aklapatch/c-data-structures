#include"hmap.h"
#include "ahash.h"
#include "test_helpers.h"
#include <stdlib.h>

int main(){
    
    uint16_t *hmap = NULL;
    hm_init(hmap, 32, realloc, ahash_buf);

    TEST_GROUP("Basic init");
    TEST_INT_EQ(hm_num(hmap), 0);
    TEST_INT_EQ(hm_err(hmap), ds_success);
    TEST_INT_EQ(hm_cap(hmap), 32);

    // insert a lot of values and see how this goes.
    uint16_t ins_vals[31];
    uint64_t keys[31];
    TEST_GROUP("Insert a few keys");
    for (uint32_t i = 0; i < sizeof(ins_vals)/sizeof(ins_vals[0]); ++i){
        ins_vals[i] = i;
        keys[i] = i;
        // insert the value
        hm_set(hmap, keys[i],ins_vals[i]);
        TEST_INT_EQ(hm_err(hmap), ds_success);

        uint16_t out_val = UINT16_MAX;
        hm_get(hmap, keys[i], out_val);
        TEST_INT_EQ(hm_err(hmap), ds_success);
        TEST_INT_EQ(out_val, ins_vals[i]);
    }

    // make sure all values didn't get overwritten
    for (uint32_t i = 0; i < sizeof(ins_vals)/sizeof(ins_vals[0]); ++i){
        uint16_t out_val = UINT16_MAX;
        hm_get(hmap, keys[i], out_val);
        TEST_INT_EQ(hm_err(hmap), ds_success);
        TEST_INT_EQ(out_val, ins_vals[i]);
    }

    TEST_GROUP("Realloc key preservation");
    hm_realloc(hmap, 64);
    // try resizing the hmap and see if all the key are still there
    for (uint32_t i = 0; i < sizeof(ins_vals)/sizeof(ins_vals[0]); ++i){
        uint16_t out_val = UINT16_MAX;
        hm_get(hmap, keys[i], out_val);
        TEST_INT_EQ(hm_err(hmap), ds_success);
        TEST_INT_EQ(out_val, ins_vals[i]);
    }

    // try deleting all the keys and make sure they're gone
    TEST_GROUP("Ensure deletion");
    for (uint32_t i = 0; i < sizeof(ins_vals)/sizeof(ins_vals[0]); ++i){
        // insert the value
        // removing key 3 causes a chain of moves that I'm still debuggin
        // removing 3 seems to work fine
        // 11 is on a list with 3 and 9, but should be at bucket 3 slot 2
        // it may be getting moved to a slot where it does not belong, which is weird
        // I need to implement direct hit tracking to fix this I think.
        hm_del(hmap, keys[i]);
        TEST_INT_EQ(hm_err(hmap), ds_success);

        uint16_t out_val = UINT16_MAX;
        hm_get(hmap, keys[i], out_val);
        TEST_INT_EQ(hm_err(hmap), ds_not_found);
    }

    TEST_GROUP("hmap free");
    hm_free(hmap);
    TESTPTREQ(hmap, NULL);

    TEST_GROUP("Bulk insert");
    hm_init(hmap, 32, realloc, ahash_buf);
    // insert a stupid number of keys and see if it still works
    for (uint32_t i = 0; i < UINT16_MAX; ++i){
        hm_set(hmap, i, i);
        TEST_INT_EQ(hm_err(hmap), ds_success);
    }
    for (uint32_t i = 0; i < UINT16_MAX; ++i){
        uint16_t out_val = UINT16_MAX;
        hm_get(hmap, i, out_val);
        TEST_INT_EQ(hm_err(hmap), ds_success);
        TEST_INT_EQ(out_val, i);
    }

    return 0;
}
