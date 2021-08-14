#include"hmap.h"
#include "test_helpers.h"

int main(){
    
    uint16_t *hmap = NULL;
    hm_realloc(hmap, 32);

    TEST("hm_num @ init", hm_num(hmap) == 0);
    TEST("hm_cap @ init w 32", hm_cap(hmap) == 32);

    uint16_t test_val = 64;
    uint64_t key = 72;
    hm_set(hmap, key, test_val);
    TESTERRSUCCESS("hm_set()", hmap);

    uint16_t searched_val = 0;
    hm_get(hmap, key, &searched_val);
    TESTERRSUCCESS("hm_get()", hmap);
    TEST("Search works for inserted val", searched_val == test_val);

    // insert a lot of values and see how this goes.
    uint16_t ins_vals[31];
    uint64_t keys[31];
    for (int i = 0; i < sizeof(ins_vals)/sizeof(ins_vals[0]); ++i){
        ins_vals[i] = i;
        keys[i] = i+1;
        // insert the value
        hm_set(hmap, keys[i],ins_vals[i]);
        printf("i=%d key=%lu val=%u\n", i, keys[i], ins_vals[i]);
        TESTERRSUCCESS("hm fill up", hmap);

        uint16_t out_val = UINT16_MAX;
        printf("i=%d key=%lx val=%x\n", i, keys[i], out_val);
        hm_get(hmap, keys[i], &out_val);
        TESTERRSUCCESS("hm query", hmap);
        TEST("hm query result", out_val == ins_vals[i]);
    }
    for (int i = 0; i < sizeof(ins_vals)/sizeof(ins_vals[0]); ++i){
        // insert the value
        uint16_t out_val = UINT16_MAX;
        hm_get(hmap, keys[i], &out_val);
        printf("i=%d key=%lx val=%x\n", i, keys[i], out_val);
        TESTERRSUCCESS("hm query", hmap);
        TEST("hm query result", out_val == ins_vals[i]);
    }

    return 0;
}
