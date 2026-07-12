/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E2AP_CRITICALITY_DIAGNOSTICS_H
#define E2AP_CRITICALITY_DIAGNOSTICS_H

#include <stddef.h>
#include <stdint.h>

#include "ric_gen_id.h"
#include "e2ap_criticality.h"
#include "ric_gen_id.h"
#include "e2ap_ie_criticality_diagnostics.h"


typedef enum {
  INITIATING_MESSAGE,
  SUCCESSFUL_OUTCOME,
  UNSUCCESSFUL_OUTCOME
} triggering_message_e;


typedef struct {
  uint8_t* procedure_code; //optional
  triggering_message_e* trig_msg; // optional
  criticality_e* proc_crit; // optional
  ric_gen_id_t* req_id; // optional
  ie_criticality_diagnostics_t* ie;
  size_t len_ie;
} criticality_diagnostics_t;

bool eq_criticality_diagnostics(const criticality_diagnostics_t* m0, const criticality_diagnostics_t* m1);

#endif

