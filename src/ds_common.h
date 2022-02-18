#pragma once

typedef void *(*realloc_fn_t)(void *,size_t);

typedef enum ds_error_e {
    ds_success = 0,
    ds_alloc_fail,
    ds_out_of_bounds,
    ds_null_ptr,
    ds_bad_param,
    ds_not_found,
    ds_too_small,
    ds_unimp, // function not implemented
    ds_wrong_ds, // used a hashmap function on an array or vice versa
    ds_fail, // That one error code you hate getting because you don't know what went wrong
    ds_num_errors,
} ds_error_e;

#define RET_SWITCH_STR(x) case (x): return (#x);

char * ds_get_err_str(ds_error_e err){
    switch (err){
        RET_SWITCH_STR(ds_success);
        RET_SWITCH_STR(ds_alloc_fail);
        RET_SWITCH_STR(ds_out_of_bounds);
        RET_SWITCH_STR(ds_null_ptr);
        RET_SWITCH_STR(ds_bad_param);
        RET_SWITCH_STR(ds_not_found);
        RET_SWITCH_STR(ds_too_small);
        RET_SWITCH_STR(ds_unimp);
        RET_SWITCH_STR(ds_wrong_ds);
        RET_SWITCH_STR(ds_fail);
        RET_SWITCH_STR(ds_num_errors);
        default:
            return "No matching error found!\n";
    }
}
