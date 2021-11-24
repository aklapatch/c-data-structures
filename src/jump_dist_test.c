#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


uintptr_t test_fn(uint8_t in){
    uintptr_t top_half = (in & 0xf0) >> 4;
    uintptr_t lower_half = in & 0xf;
    return in << top_half | in;
}

int main(){

    for (uint8_t i = 0; i < UINT8_MAX; ++i){
        uintptr_t out_val = test_fn(i);
        printf("i=%u val=%lu\n", i, out_val);
    }
    return 0;
}
