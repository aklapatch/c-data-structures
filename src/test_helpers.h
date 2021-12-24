#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// These test tools are intended to be added in an ad hoc fashion.
// you can set up your test program to make sure something works, and then add
// These macros to make things a little fancier or easier.
// These are intended to be easy to add, and unlike a testing framework, they 
// don't require you to refactor your code a lot to add them in.

// print out a test group label
#define TEST_GROUP(label) printf("[Test Group]: %s\n", label)

#define TEST_GROUP_OK() printf("[Test Group]: OK\n")

// We only want the expression (val1 or val2) to be evaluated once, but
// we still want to get the value of the result of the function.
// We also want the stringified version of the expression, which 
// We can only get from a macro.
// In the printf, s stands for signed and u stands for unsigned 
#define TEST_CMP(val1, val2, cmp_expr)\
    do{\
        uintptr_t _val1 = (val1), _val2 = (val2);\
        if (_val1 cmp_expr  _val2){\
            printf("[Test]: %s @ %u FAIL! %s (0x%lX, u %lu, s %ld) %s %s (0x%lX, u %lu, s %ld)\n", __FILE__, __LINE__, #val1, _val1, _val1, _val1, #cmp_expr, #val2, _val2, _val2, _val2);\
            exit(1);\
        }\
    }while(0)

#define TEST_INT_EQ(val1, val2) TEST_CMP(val1, val2, !=)
#define TEST_INT_NEQ(val1, val2) TEST_CMP(val1, val2, ==)

// ahhhh C, reducing code duplication by casting pointers to integers
// (sarcastic)
#define TEST_PTR_EQ(ptr1, ptr2) TEST_INT_EQ((uintptr_t)ptr1, (uintptr_t)ptr2)
#define TEST_PTR_NEQ(ptr1, ptr2) TEST_INT_NEQ((uintptr_t)ptr1, (uintptr_t)ptr2)

// return UINTPTR_MAX if equal
uintptr_t where_uneql(void *ptr1, void *ptr2, size_t n){
    uint8_t *ptr1_8 = (uint8_t*)ptr1, *ptr2_8 = (uint8_t*)ptr2;
    for (uintptr_t i = 0; i < n; ++i){
        if (ptr1_8[i] != ptr2_8[i]){
            printf("[Test]: Elements %lu differ %x != %x\n", i, ptr1_8[i], ptr2_8[i]);
            return i;
        }
    }
    return UINTPTR_MAX;
}

#define TEST_MEM_EQL(ptr1, ptr2, n)\
    do{\
        uintptr_t _diff_i = where_uneql(ptr, ptr2,n);\
        if (_diff_i != UINTPTR_MAX){\
            printf("[Test]: %s @ %u FAIL!\n", __FILE__, __LINE__);\
        }\
    } while (0)

