/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E2_NODE_CONNECTION_UPDATE_FAILURE_H
#define E2_NODE_CONNECTION_UPDATE_FAILURE_H 

#include "common/e2ap_criticality_diagnostics.h"
#include "common/e2ap_cause.h"
#include "common/e2ap_time_to_wait.h"

typedef struct{
  cause_t cause;
  e2ap_time_to_wait_e* time_wait; 
  criticality_diagnostics_t* crit_diag; 
} e2_node_connection_update_failure_t;

#endif

