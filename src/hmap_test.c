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
    uint16_t searched_val = hmap[hm_find_item(hmap, key, sizeof(*hmap))];
    TEST("Search works for inserted val", searched_val == test_val);

    // insert a lot of values and see how this goes.
    uint16_t ins_vals[31];
    uint64_t keys[31];
    for (int i = 0; i < sizeof(ins_vals)/sizeof(ins_vals[0]); ++i){
        ins_vals[i] = i;
        keys[i] = i+3;
        // insert the value
        hm_set(hmap, keys[i],ins_vals[i]);
        printf("i = %d\n", i);
        TESTERRSUCCESS("hm fill up", hmap);
    }

    return 0;
}
