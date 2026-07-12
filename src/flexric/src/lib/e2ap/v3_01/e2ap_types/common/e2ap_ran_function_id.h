/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E2AP_RAN_FUNCTION_ID_H
#define E2AP_RAN_FUNCTION_ID_H

#include <stdbool.h>
#include <stdint.h>

typedef struct{
  uint16_t id;
  uint16_t rev;
} ran_function_id_t;

bool eq_ran_function_id(const ran_function_id_t* m0, const ran_function_id_t* m1);

#endif

