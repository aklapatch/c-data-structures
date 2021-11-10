#include "ahash.h"
#include <stdio.h>  
#define ROUNDS (3*UINT16_MAX)

int main(){
    // test distribution of the ahash function

    uint64_t start_val = 0xf32341234;
    uint64_t test_val = ahash_buf((uint8_t*)&start_val, sizeof(start_val));
    uint64_t final_second_hash = ahash_buf((uint8_t*)&test_val, sizeof(test_val));
    printf("test_val = %lx final_second_hash = %lx\n", test_val, final_second_hash);

    uint64_t hashes[ROUNDS] = {0};
    for (uint64_t i = 0; i < ROUNDS; ++i){
        hashes[i] = ahash_buf((uint8_t*)&i, sizeof(i));
    }
    for (uint64_t i = 0; i < ROUNDS; ++i){
        //compare different matches and see if one was found
        for (uint64_t j = 0; j < ROUNDS; ++j){
            if (j != i && hashes[i] == hashes[j]){
                printf("Collision found! %lx i = %lu j = %lu\n", hashes[i], i,j);
                return 1;
            }
        }
    }

    return 0;
}
