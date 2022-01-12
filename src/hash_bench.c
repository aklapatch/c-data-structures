#include "ahash.h"
#include "xxhash.h"
#include "ant_hash.h"
#include <stdio.h>  
#include <time.h>
#define ROUNDS (5*UINT16_MAX)

// a function to keep the compiler from optimizing away stuff
uint64_t do_nothing(uint64_t *in){
    (void)in;
    return 0;
}
uint64_t sum(uint64_t *in, uint64_t num){
    num--;
    uint64_t sum = 0;
    for (; num > 0; --num){
        sum += in[num];
    }
    return sum;
}

int main(){
    uint64_t hashes[ROUNDS] = {0};

    printf("Starting ahash test\n");

    clock_t time = clock();
    for (uint64_t i = 0; i < ROUNDS; ++i){
        hashes[i] = ahash_buf(&i, sizeof(i));
    }
    time = clock() - time;
    // sum all the hashes so the compiler doesn' t
    printf("%u 8 bytes hashes took %g sec %lu clocks sum=%lu\n", ROUNDS , (double)(time)/CLOCKS_PER_SEC, time, sum(hashes, ROUNDS));

    printf("Starting xxhash test\n");

    time = clock();
    for (uint64_t i = 0; i < ROUNDS; ++i){
        hashes[i] = xxhash_buf(&i, sizeof(i));
    }
    time = clock() - time;

    printf("%u 8 bytes hashes took %g sec %lu clocks sum=%lu\n", ROUNDS , (double)(time)/CLOCKS_PER_SEC, time, sum(hashes, ROUNDS));

    printf("Starting ant_hash test\n");
    time = clock();
    for (uint64_t i = 0; i < ROUNDS; ++i){
        hashes[i] = ant_hash(&i, sizeof(i));
    }
    time = clock() - time;

    printf("%u 8 bytes hashes took %g sec %lu clocks sum=%lu\n", ROUNDS , (double)(time)/CLOCKS_PER_SEC, time, sum(hashes, ROUNDS));

    return 0;
}
