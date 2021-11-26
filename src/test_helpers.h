#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define ITEMS_IN_ARR(arr) (sizeof((arr))/sizeof((arr)[0]))
#define TESTGROUP(label) printf("[Test Group]: %s\n", label);
#define TEST(label, expression)\
    do{\
        if (!(expression)){\
            printf("[Test]: (%s) FAIL! (%s) is false @ %s:%u\n",label, #expression, __FILE__, __LINE__);\
            exit(1);\
        }\
    } while(0)


#define TESTINTEQ(val1, val2)\
    do{\
        if (!test_int_eq((val1), (val2), #val1, #val2)){ exit(1); } \
    }while(0)

#define TESTPTREQ(ptr1, ptr2) TESTINTEQ((uintptr_t)ptr1, (uintptr_t)ptr2)

bool test_int_eq(uintptr_t val1, uintptr_t val2, char *fail_str1, char *fail_str2){
    if (val1 != val2){
        printf("[test]: FAIL %s = %lx, %s = %lx\n", fail_str1, val1, fail_str2, val2);
        return false;
    }
    return true;
}

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
