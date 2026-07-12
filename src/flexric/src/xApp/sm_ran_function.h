/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef SM_RAN_FUNCTION_MIR_H
#define SM_RAN_FUNCTION_MIR_H

#include <stdint.h>
#include "sm_ran_function_def.h"

typedef struct{
  sm_ran_function_def_t defn;
  uint16_t id;
  uint16_t rev;
#ifdef E2AP_V1
  byte_array_t* oid; // optional
#elif defined(E2AP_V2) || defined(E2AP_V3) 
  byte_array_t oid; 
#endif
} sm_ran_function_t;

void free_sm_ran_function(sm_ran_function_t* src);

sm_ran_function_t cp_sm_ran_function(sm_ran_function_t const* src);

bool eq_sm_ran_function(sm_ran_function_t const* m0, sm_ran_function_t const* m1);

#endif

