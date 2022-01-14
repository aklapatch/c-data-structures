#include "ahash.h"
#include "xxhash.h"
#include "ant_hash.h"
#include <stdio.h>  
#include <time.h>
#define ROUNDS (9*UINT16_MAX)

int main(){
    uint64_t hash = 0;

    printf("Starting ahash test\n");

    clock_t time = clock();
    for (uint64_t i = 0; i < ROUNDS; ++i){
        hash = ahash_buf(&hash, sizeof(hash));
    }
    time = clock() - time;
    // sum all the hashes so the compiler doesn' t
    printf("%u 8 bytes hashes took %g sec %lu clocks sum=%lu\n", ROUNDS , (double)(time)/CLOCKS_PER_SEC, time, hash);

    printf("Starting xxhash test\n");

    time = clock();
    for (uint64_t i = 0; i < ROUNDS; ++i){
        hash = xxhash_buf(&hash, sizeof(hash));
    }
    time = clock() - time;

    printf("%u 8 bytes hashes took %g sec %lu clocks sum=%lu\n", ROUNDS , (double)(time)/CLOCKS_PER_SEC, time, hash);

    printf("Starting ant_hash test\n");
    time = clock();
    for (uint64_t i = 0; i < ROUNDS; ++i){
        hash = ant_hash(hash);
    }
    time = clock() - time;

    printf("%u 8 bytes hashes took %g sec %lu clocks sum=%lu\n", ROUNDS , (double)(time)/CLOCKS_PER_SEC, time, hash);

    return 0;
}
