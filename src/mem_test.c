#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#include "test_helpers.h"
#include "mem.h"

#define SIZE (3*4096)
#define TIMES (100)

int main(){
    // use 4 byte vals to get 4 byte alignment`
    uint32_t in_buf[SIZE/4];
    uint32_t out_buf[SIZE/4];
    memset(in_buf, 0xff, sizeof(in_buf));
    memset(out_buf, 0, sizeof(out_buf));

    uint8_t *in_ptr = (uint8_t*)in_buf, *out_ptr = (uint8_t*)out_buf;

    clock_t n_cp_time = 0, cp_time = 0, set_time = 0, n_set_time = 0;

#define SAVE_TIME(code, save_val)\
    do{\
        clock_t tmp = clock();\
        code;\
        save_val += clock() - tmp;\
    } while (0)

    // run both copies a bunch of times
    for (uint32_t i = 0; i < TIMES; ++i){
        SAVE_TIME(
            naive_memcpy(out_buf, in_buf, SIZE),
            n_cp_time);

        memset(out_buf, 0, sizeof(out_buf));

        SAVE_TIME(
                naive_memset(out_buf, 0xee, SIZE),
                n_set_time);

        memset(out_buf, 0, sizeof(out_buf));

        SAVE_TIME(
                mo_memcpy(out_buf, in_buf, SIZE),
                cp_time);

        memset(out_buf, 0, sizeof(out_buf));

        SAVE_TIME(
                mo_memset(out_buf, 0xee, SIZE),
                set_time);

        memset(out_buf, 0, sizeof(out_buf));
    }

    printf("Naive cp: clocks=%lu time=%f\n", n_cp_time, (double)n_cp_time/CLOCKS_PER_SEC);
    printf("Naive set: clocks=%lu time=%f\n", n_set_time, (double)n_set_time/CLOCKS_PER_SEC);
    printf("cp: clocks=%lu time=%f\n", cp_time, (double)cp_time/CLOCKS_PER_SEC);
    printf("set: clocks=%lu time=%f\n", set_time, (double)set_time/CLOCKS_PER_SEC);

    return 0;
}
