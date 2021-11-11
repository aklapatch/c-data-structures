#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define ITEMS_IN_ARR(arr) (sizeof((arr))/sizeof((arr)[0]))
#define TEST(label, expression)\
    do{\
        printf("[Test]: (%s): ", label);\
        if (expression){\
            printf(" OK\n");\
        } else {\
            printf(" FAIL! (%s) is false @ %s:%u\n", #expression, __FILE__, __LINE__);\
            exit(1);\
        }\
    } while(0)

#define TESTERRSUCCESS(message, ptr)\
    do{\
        TEST(message " err val check", dynarr_err(ptr) == ds_success);\
        TEST(message " err set check", !dynarr_is_err_set(ptr));\
    }while (0)

#define TESTERRFAIL(message, ptr, err_val)\
    do{\
        TEST(message " err val check", dynarr_err(ptr) == err_val);\
        TEST(message " err set check", dynarr_is_err_set(ptr));\
    }while (0)
