#include"hmap.h"
#include "ahash.h"
#include "test_helpers.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#define TIMES (3*UINT16_MAX)
#define RNDS (10)
// bench insertion, lookup, at least.
// Benching deletion doesn't make too much sense.
int main(){

    // init hmap to minimum size with a reasonable sized payload type
    clock_t ins_avg = 0, query_avg = 0;
    for (uint8_t j = RNDS; j > 0; --j){
        uint32_t *hmap = NULL;
        hm_init(hmap, 16, realloc, ahash_buf);

        clock_t start = clock();
        for (uint32_t i = 0; i < TIMES; ++i){
            hm_set(hmap, i, i);
            if (hm_is_err_set(hmap)){
                printf("Insert failed!\n");
                exit(1);
            }
        }
        clock_t end = clock();
        ins_avg += end-start;

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

        end = clock();
        query_avg += end - start;

        hm_free(hmap);
    }

    query_avg /= RNDS;
    ins_avg /= RNDS;
    printf("%u insertions took %g sec %lu clocks avg over %u runs\n",TIMES, (double)(ins_avg)/CLOCKS_PER_SEC, ins_avg, RNDS);
    printf("%u qeuries took %g sec %lu clocks avg over %u runs\n",TIMES, (double)(query_avg)/CLOCKS_PER_SEC, query_avg, RNDS);

    return 0;
}
