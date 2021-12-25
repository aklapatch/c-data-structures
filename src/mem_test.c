#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#include "test_helpers.h"
#include "mem.h"

#define SIZE (3*4096)
#define TIMES (200)

int main(){
    // use 4 byte vals to get 4 byte alignment`
    uint32_t in_buf[SIZE/4];
    uint32_t out_buf[SIZE/4];
    uint32_t ref_buf[SIZE/4];
    memset(in_buf, 0xff, sizeof(in_buf));
    memset(out_buf, 0, sizeof(out_buf));
    memset(ref_buf, 0xee, sizeof(ref_buf));

#define SAVE_TIME(code, save_val)\
    do{\
        clock_t tmp = clock();\
        code;\
        save_val += clock() - tmp;\
    } while (0)

    // run both copies a bunch of times 
    // with various sizes
    uint32_t sizes[] = { SIZE, 4096, 2048, 1024, 512, 256, 128, 64, 32, 16, 8, 4 };
    for (uint8_t k = 0; k < sizeof(sizes)/sizeof(uint32_t); ++k){
        uint32_t buf_size = sizes[k];
        uint8_t *in_ptr = (uint8_t*)in_buf, *out_ptr = (uint8_t*)out_buf, *ref_ptr = (uint8_t*)ref_buf;
        for (uint8_t j = 0; j < 4; ++j){
            clock_t n_cp_time = 0, cp_time = 0, set_time = 0, n_set_time = 0;
            for (uint32_t i = 0; i < TIMES; ++i){
                memset(out_ptr, 0, buf_size);

                SAVE_TIME(
                        naive_memcpy(out_ptr, in_ptr, buf_size),
                        n_cp_time);

                memset(out_ptr, 0, buf_size);

                SAVE_TIME(
                        mo_memcpy(out_ptr, in_ptr, buf_size),
                        cp_time);

                TEST_MEM_EQ(out_ptr, in_ptr, buf_size);

                memset(out_ptr, 0, buf_size);
                SAVE_TIME(
                        naive_memset(out_ptr, ref_ptr[0], buf_size),
                        n_set_time);

                memset(out_ptr, 0, buf_size);
                SAVE_TIME(
                        mo_memset(out_ptr, ref_ptr[0], buf_size),
                        set_time);

                TEST_MEM_EQ(out_ptr, ref_ptr, buf_size);
            }

            printf("alignment=%lu size=%u\n", (uintptr_t)ref_ptr % 4, buf_size);
            printf("Naive cp: clocks=%lu time=%f. ", n_cp_time, (double)n_cp_time/CLOCKS_PER_SEC);
            printf("Naive set: clocks=%lu time=%f\n", n_set_time, (double)n_set_time/CLOCKS_PER_SEC);
            printf("      cp: clocks=%lu time=%f. ", cp_time, (double)cp_time/CLOCKS_PER_SEC);
            printf("      set: clocks=%lu time=%f\n", set_time, (double)set_time/CLOCKS_PER_SEC);

            // change the alignment to test different modes
            ++ref_ptr; ++in_ptr; ++out_ptr;
            if (sizes[k] == SIZE){
                --buf_size;
            }
        }
    }
    return 0;
}
