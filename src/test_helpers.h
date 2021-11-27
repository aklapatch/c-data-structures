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

// This macro passing to a function is for a specific reason.
// We only want the expression (val1 or val2) to be evaluated once, but
// we still want to get the value of the result of the function.
// We also want the stringified version of the expression, which 
// We can only get from a macro.
// I guess I could use variables, and that may be better.
// The values are in the funciton, so they should be printed there
// To associate the value with the string, pass the string into the function
#define TESTINTNEQ(val1, val2)\
    do{\
        if (test_int_eq((val1), (val2), #val1, #val2, __FILE__, __LINE__)){  exit(1); }\
    }while(0)


#define TESTINTEQ(val1, val2)\
    do{\
        uintptr_t _val1 = (val1), _val2 = (val2);\
        if (_val1 != _val2){\
            printf("[Test]: %s @ %u FAIL! %s = %lx, %s = %lx \n", __FILE__, __LINE__, #val1, _val1, #val2, _val2);\
            exit(1);\
        }\
    }while(0)

// C, reducing code duplication by casting pointers to integers
// (sarcastic)
#define TESTPTREQ(ptr1, ptr2) TESTINTEQ((uintptr_t)ptr1, (uintptr_t)ptr2)

#define TESTINTNEQ(val1, val2)\
    do{\
        uintptr_t _val1 = (val1), _val2 = (val2);\
        if (_val1 == _val2){\
            printf("[Test]: %s @ %u FAIL! %s = %lx, %s = %lx \n", __FILE__, __LINE__, #val1, _val1, #val2, _val2);\
            exit(1);\
        }\
    }while(0)

#define TESTPTRNEQ(ptr1, ptr2) TESTINTNEQ((uintptr_t)ptr1, (uintptr_t)ptr2)

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
