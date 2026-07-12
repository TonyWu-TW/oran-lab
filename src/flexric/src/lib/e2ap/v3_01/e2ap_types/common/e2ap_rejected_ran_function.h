/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E2AP_REJECTED_RAN_FUNCTION_H
#define E2AP_REJECTED_RAN_FUNCTION_H

#include <stdint.h>
#include "e2ap_cause.h"

// inside of E2 Setup msgs
typedef struct rejected_ran_function {
  uint16_t id;
  cause_t cause;
} rejected_ran_function_t;

bool eq_rejected_ran_function(const rejected_ran_function_t* m0, const rejected_ran_function_t* m1);

#endif
