#include"hmap.h"
#include "ahash.h"
#include "test_helpers.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#define TIMES (3*UINT16_MAX)
#define RNDS (40)
// bench insertion, lookup, at least.
// Benching deletion doesn't make too much sense.
int main(){

    // init hmap to minimum size with a reasonable sized payload type
    clock_t ins_tot = 0, query_tot = 0;
    for (uint8_t j = RNDS; j > 0; --j){
        uint32_t *hmap = NULL;
        hm_init(hmap, 16, realloc, xxhash_buf);

        clock_t start = clock();
        for (uint32_t i = 0; i < TIMES; ++i){
            hm_set(hmap, i, i);
            if (hm_is_err_set(hmap)){
                printf("Insert failed!\n");
                exit(1);
            }
        }
        ins_tot += clock() - start;

        // search for all the keys we inserted.
        start = clock();
        for (uint32_t i = 0; i < TIMES; ++i){
            uint32_t out_val = UINT32_MAX;
            hm_get(hmap, i, out_val);
            if (hm_is_err_set(hmap)){
                printf("Insert failed!\n");
                exit(1);
            }
        }

        query_tot += clock() - start;

        hm_free(hmap);
    }

    printf("%u insertions took %g sec %lu clocks over %u runs\n",TIMES, (double)(ins_tot)/CLOCKS_PER_SEC, ins_tot, RNDS);
    printf("%u qeuries took %g sec %lu clocks over %u runs\n",TIMES, (double)(query_tot)/CLOCKS_PER_SEC, query_tot, RNDS);

    return 0;
}
