#include "ahash.h"
#include "xxhash.h"
#include "ant_hash.h"
#include <stdio.h>  
#include <time.h>
#include <stdbool.h>
#define ROUNDS (2*UINT16_MAX)

bool check_hashes(uint64_t *in, uint64_t num){
    for (uint64_t i = 0; i < num; ++i){
        //compare different matches and see if one was found
        // start at i + 1 since the ith element has already been compared against the jth element
        for (uint64_t j = i + 1; j < num; ++j){
            if (j != i && in[i] == in[j]){
                printf("Collision found! %lx i = %lu j = %lu\n", in[i], i, j);
                return true;
            }
        }
    }
    return false;
}

int main(){
    uint64_t hashes[ROUNDS] = {0};

    printf("Starting ahash test\n");

    clock_t time = clock();
    for (uint64_t i = 0; i < ROUNDS; ++i){
        hashes[i] = ahash_buf(&i, sizeof(i));
    }
    time = clock() - time;
    printf("%u 8 bytes hashes took %g sec %lu clocks\n", ROUNDS , (double)(time)/CLOCKS_PER_SEC, time);

    if (check_hashes(hashes, ROUNDS)){
        return 1;
    }

    printf("Starting xxhash test\n");

    time = clock();
    for (uint64_t i = 0; i < ROUNDS; ++i){
        hashes[i] = xxhash_buf(&i, sizeof(i));
    }
    time = clock() - time;
    printf("%u 8 bytes hashes took %g sec %lu clocks\n", ROUNDS , (double)(time)/CLOCKS_PER_SEC, time);

    if (check_hashes(hashes, ROUNDS)){
        return 1;
    }

    printf("Starting ant_hash test\n");

    time = clock();
    for (uint64_t i = 0; i < ROUNDS; ++i){
        hashes[i] = ant_hash(i);
    }
    time = clock() - time;
    printf("%u 8 bytes hashes took %g sec %lu clocks\n", ROUNDS , (double)(time)/CLOCKS_PER_SEC, time);

    if (check_hashes(hashes, ROUNDS)){
        return 1;
    }

    return 0;
}
