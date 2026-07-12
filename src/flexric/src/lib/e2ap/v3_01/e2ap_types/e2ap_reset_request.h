/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef E2AP_RESET_REQUEST_H
#define E2AP_RESET_REQUEST_H

#include "common/e2ap_cause.h"
#include <stdint.h>

typedef struct {
  uint8_t trans_id;
  cause_t cause;
} e2ap_reset_request_t;

bool eq_reset_request(const e2ap_reset_request_t* m0, const e2ap_reset_request_t* m1);

#endif

