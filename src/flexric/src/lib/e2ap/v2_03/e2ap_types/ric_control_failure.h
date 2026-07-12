/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef RIC_CONTROL_FAILURE_H
#define RIC_CONTROL_FAILURE_H

#include "common/ric_gen_id.h"
#include "common/e2ap_cause.h" 
#include "common/e2ap_criticality_diagnostics.h"
#include "util/byte_array.h"

typedef struct {
  ric_gen_id_t ric_id;
  byte_array_t* call_process_id; // optional
  cause_t cause;
  byte_array_t* control_outcome; // optional
  criticality_diagnostics_t* crit; // optional
} ric_control_failure_t;

bool eq_control_failure(const ric_control_failure_t* m0, const ric_control_failure_t* m1);

ric_control_failure_t copy_ric_control_failure(const ric_control_failure_t* src);

#endif

