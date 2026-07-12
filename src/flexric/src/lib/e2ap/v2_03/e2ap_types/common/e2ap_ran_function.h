/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E2AP_RAN_FUNCTION_H
#define E2AP_RAN_FUNCTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../../../../util/byte_array.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct ran_function {
  byte_array_t defn;
  uint16_t id;
  uint16_t rev;
  byte_array_t oid; 
} ran_function_t;

ran_function_t cp_ran_function(const ran_function_t* src);

void free_ran_function_wrapper(void* a);

void free_ran_function(ran_function_t* src);

bool eq_ran_function(const ran_function_t* m0, const ran_function_t* m1);

bool eq_ran_function_wrapper(void const* a_v, void const* b_v);

int cmp_ran_function(const ran_function_t* m0, const ran_function_t* m1);

#ifdef __cplusplus
}
#endif

#endif
