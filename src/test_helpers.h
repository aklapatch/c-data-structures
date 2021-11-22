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

#define TESTERRSUCCESS(message, ptr, is_hm)\
    do{\
        TEST(message " err val check", is_hm ? hm_err(ptr) == ds_success : dynarr_err(ptr) == ds_success);\
        TEST(message " err set check", is_hm ? !hm_is_err_set(ptr) : !dynarr_is_err_set(ptr));\
    }while (0)

#define TESTERRFAIL(message, ptr, err_val, is_hm)\
    do{\
        TEST(message " err val check", is_hm ? hm_err(ptr) == err_val : dynarr_err(ptr) == err_val);\
        TEST(message " err set check", is_hm ? hm_is_err_set(ptr) : dynarr_is_err_set(ptr));\
    }while (0)
