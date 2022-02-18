#include <stdlib.h>
#include "hmap.h"
#include "test_helpers.h"

#define NUM_KEYS (31)
int main(){
    
    uint16_t *hmap = NULL;
    hm_init(hmap, 32, realloc, ant_hash);

    TEST_GROUP("Basic init");
    TEST_INT_EQ(hm_num(hmap), 0);
    TEST_INT_EQ(hm_err(hmap), ds_success);
    TEST_INT_EQ(hm_cap(hmap), 32);
    TEST_GROUP_OK();

    // insert a lot of values and see how this goes.
    TEST_GROUP("Insert a few keys");
    for (uint16_t i = 0; i < NUM_KEYS; ++i){
        printf("key=%u\n", i);
        // insert the value
        hm_set(hmap, i, i);
        TEST_INT_EQ(hm_err(hmap), ds_success);

        uint16_t out_val = UINT16_MAX;
        hm_get(hmap, i, out_val);
        TEST_INT_EQ(hm_err(hmap), ds_success);
        TEST_INT_EQ(out_val, i);
    }

    // make sure all values didn't get overwritten
    for (uint16_t i = 0; i < NUM_KEYS; ++i){
        uint16_t out_val = UINT16_MAX;
        hm_get(hmap, i, out_val);
        TEST_INT_EQ(hm_err(hmap), ds_success);
        TEST_INT_EQ(out_val, i);
    }
    TEST_GROUP_OK();

    TEST_GROUP("Realloc key preservation");
    hm_realloc(hmap, 64);
    // try resizing the hmap and see if all the key are still there
    for (uint16_t i = 0; i < NUM_KEYS; ++i){
        uint16_t out_val = UINT16_MAX;
        hm_get(hmap, i, out_val);
        TEST_INT_EQ(hm_err(hmap), ds_success);
        TEST_INT_EQ(out_val, i);
    }
    TEST_GROUP_OK();

    // try deleting all the keys and make sure they're gone
    TEST_GROUP("Ensure deletion");
    for (uint16_t i = 0; i < NUM_KEYS; ++i){
        printf("1 del key=%u\n", i);
        hm_del(hmap, i);
        TEST_INT_EQ(hm_err(hmap), ds_success);

        uint16_t out_val = UINT16_MAX;
        hm_get(hmap, i, out_val);
        TEST_INT_EQ(hm_err(hmap), ds_not_found);
    }
    TEST_INT_EQ(hm_num(hmap), 0);
    TEST_GROUP_OK();

    TEST_GROUP("hmap free");
    hm_free(hmap);
    TEST_PTR_EQ(hmap, NULL);
    TEST_GROUP_OK();

    TEST_GROUP("Bulk insert");
    hm_init(hmap, 32, realloc, ant_hash);
    // insert a stupid number of keys and see if it still works
    for (uint16_t i = 0; i < UINT16_MAX; ++i){
        printf("key=%u\n", i);
        hm_set(hmap, i, i);
        TEST_INT_EQ(hm_err(hmap), ds_success);

        uint16_t out_val = UINT16_MAX;
        hm_get(hmap, i, out_val);
        TEST_INT_EQ(hm_err(hmap), ds_success);
        TEST_INT_EQ(out_val, i);
    }
    for (uint16_t i = 0; i < UINT16_MAX; ++i){
        uint16_t out_val = UINT16_MAX;
        printf("key=%u\n", i);
        hm_get(hmap, i, out_val);
        TEST_INT_EQ(hm_err(hmap), ds_success);
        TEST_INT_EQ(out_val, i);
    }
    TEST_GROUP_OK();

    TEST_GROUP("Bulk delete");
    // try deleting everything and then see if the keys are still there
    for (uint16_t i = 0; i < UINT16_MAX; ++i){
        hm_del(hmap, i);
        TEST_INT_EQ(hm_err(hmap), ds_success);

        uint16_t out_val = UINT16_MAX;
        hm_get(hmap, i, out_val);
        TEST_INT_EQ(hm_err(hmap), ds_not_found);
    }
    TEST_INT_EQ(hm_num(hmap), 0);

    TEST_GROUP_OK();
    
    TEST_GROUP("opposite order insert + delete");
    // re-insert keys in the opposite order to see if that works
    for (uint16_t i = UINT16_MAX - 1; i > 0; --i){
        printf("key=%u\n", i);
        hm_set(hmap, i, i);
        TEST_INT_EQ(hm_err(hmap), ds_success);

        uint16_t out_val = UINT16_MAX;
        hm_get(hmap, i, out_val);
        TEST_INT_EQ(hm_err(hmap), ds_success);
        TEST_INT_EQ(out_val, i);
    }

    uint16_t stop_val = UINT16_MAX/16;
    for (uint16_t i = stop_val; i > 0 ; --i){
        printf("del key=%u\n", i);
        hm_del(hmap, i);
        TEST_INT_EQ(hm_err(hmap), ds_success);

        // query all the values that should still be there
        for (uint16_t j = (i > 0) ? i - 1 : 0; j > 0 ; --j){
            uint16_t out_val = UINT16_MAX;
            hm_get(hmap, j, out_val);
            TEST_INT_EQ(hm_err(hmap), ds_success);
            TEST_INT_EQ(out_val, j);
        }
    }

    TEST_GROUP_OK();
    
    hm_free(hmap);

    return 0;
}
