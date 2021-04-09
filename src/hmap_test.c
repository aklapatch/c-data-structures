#include"hmap.h"
#define ROUNDS (4*UINT16_MAX)

int main(){

    uint64_t matches[ROUNDS] = {0};
    uint32_t total_different_vals = 0;
    uint32_t collisions = 0;
    for (uint64_t i = 0; i < ROUNDS; ++i){
        bool collision_found = false;
        uint64_t hash = ahash_buf((uint8_t*)&i, sizeof(i));

        // see if any other matches have been found yet
        for (uint32_t i = 0; i < total_different_vals; ++i){
            if (hash == matches[i]){
                printf("Collision found! %lx\n", hash);
                collision_found = true;
                collisions++;
                break;
            }
        }
        if (!collision_found){
            matches[total_different_vals] = hash;
            total_different_vals++;
        }
    }
    printf("Total collisions: %u\n", collisions);

    return 0;
}
