#include"hmap.h"

int main(){
    
    uint16_t *hmap = NULL;
    hm_realloc(hmap, 32);

    printf("hmap num=%u\n",hm_num(hmap));
    printf("hmap cap=%u\n",hm_cap(hmap));

    uint16_t test_val = 64;
    uint64_t key = 72;
    hm_set(hmap, key, test_val);
    printf("hmap inserted val= %u\n", hmap[hm_find_item(hmap, key)]);



    return 0;
}
