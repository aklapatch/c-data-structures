#pragma once
#include <stdio.h>
#include <assert.h>

#define TEST(label, expression)\
    do{\
        printf("Test %s\n", label);\
        printf("Testing: (%s)", #expression);\
        assert(expression);\
        printf(": ok\n\n");\
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
        printf("Error str is \"%s\"\n\n", dynarr_err_str(ptr));\
    }while (0)
