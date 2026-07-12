/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E2AP_RAN_FUNCTION_ID_REV_H
#define E2AP_RAN_FUNCTION_ID_REV_H 

#include <stdbool.h>
#include <stdint.h>

typedef struct{
  uint16_t id;
  uint16_t rev;
} e2ap_ran_function_id_rev_t;

bool eq_ran_function_id_rev(const e2ap_ran_function_id_rev_t* m0, const e2ap_ran_function_id_rev_t* m1);

#endif
