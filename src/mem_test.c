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

    // print header for gnuplot 
    printf("size align setclocks nsetclocks cpclocks ncpclocks\n");

    // run both copies a bunch of times 
    // with various sizes
    uint32_t sizes[] = { SIZE, 4096, 2048, 1024, 512, 256, 128, 64, 32, 16, 8, 4 };
    for (uint8_t k = 0; k < sizeof(sizes)/sizeof(uint32_t); ++k){
        uint32_t buf_size = sizes[k];
        uint8_t *in_ptr = (uint8_t*)in_buf, *out_ptr = (uint8_t*)out_buf, *ref_ptr = (uint8_t*)ref_buf;
        // test different dst alignment
        for (uint32_t a = 0; a < 4; ++a,++out_ptr,++ref_ptr){
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

            // alignment, 1 means 1 byte aligned, 2 means 2 byte aligned
            // 4 means 4 byte aligned
            uintptr_t src_align = (uintptr_t)in_ptr % 4;
            uintptr_t dst_align = (uintptr_t)out_ptr % 4;
            uintptr_t align = 1;
            if (src_align == dst_align){
                align = 4;
            } else if (src_align % 2 == dst_align % 2){
                align = 2;
            }
            printf("%u %lu ", buf_size, align);
            printf("%lu ", set_time);
            printf("%lu ", n_set_time);
            printf("%lu ", cp_time);
            printf("%lu\n", n_cp_time);

            // change the alignment to test different modes
            if (sizes[k] == SIZE){
                --buf_size;
            }
        }
    }
    return 0;
}
