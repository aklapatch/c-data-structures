#include "ahash.h"
#include "xxhash.h"
#include <stdio.h>  
#include <time.h>
#define ROUNDS (1*UINT16_MAX)

int main(){
    uint64_t hashes[ROUNDS] = {0};

    printf("Starting ahash test\n");

    clock_t time = clock();
    for (uint64_t i = 0; i < ROUNDS; ++i){
        hashes[i] = ahash_buf(&i, sizeof(i));
    }
    time = clock() - time;
    printf("%u 8 bytes hashes took %g sec %lu clocks\n", ROUNDS , (double)(time)/CLOCKS_PER_SEC, time);
    for (uint64_t i = 0; i < ROUNDS; ++i){
        //compare different matches and see if one was found
        for (uint64_t j = 0; j < ROUNDS; ++j){
            if (j != i && hashes[i] == hashes[j]){
                printf("Collision found! %lx i = %lu j = %lu\n", hashes[i], i,j);
                return 1;
            }
        }
    }

    printf("Starting xxhash test\n");

    time = clock();
    for (uint64_t i = 0; i < ROUNDS; ++i){
        hashes[i] = xxhash_buf(&i, sizeof(i));
    }
    time = clock() - time;
    printf("%u 8 bytes hashes took %g sec %lu clocks\n", ROUNDS , (double)(time)/CLOCKS_PER_SEC, time);

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
