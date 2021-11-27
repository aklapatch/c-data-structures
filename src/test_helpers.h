#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define TEST_GROUP(label) printf("[Test Group]: %s\n", label);

// We only want the expression (val1 or val2) to be evaluated once, but
// we still want to get the value of the result of the function.
// We also want the stringified version of the expression, which 
// We can only get from a macro.
#define TEST_CMP(val1, val2, cmp_expr)\
    do{\
        uintptr_t _val1 = (val1), _val2 = (val2);\
        if (_val1 cmp_expr  _val2){\
            printf("[Test]: %s @ %u FAIL! %s (%lx) %s %s (%lx)\n", __FILE__, __LINE__, #val1, _val1, #cmp_expr, #val2, _val2);\
            exit(1);\
        }\
    }while(0)

#define TEST_INT_EQ(val1, val2) TEST_CMP(val1, val2, !=)
#define TEST_INT_NEQ(val1, val2) TEST_CMP(val1, val2, ==)

// ahhhh C, reducing code duplication by casting pointers to integers
// (sarcastic)
#define TEST_PTR_EQ(ptr1, ptr2) TEST_INT_EQ((uintptr_t)ptr1, (uintptr_t)ptr2)

#define TEST_PTR_NEQ(ptr1, ptr2) TEST_INT_NEQ((uintptr_t)ptr1, (uintptr_t)ptr2)
