/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef MIR_SM_RIC_H
#define MIR_SM_RIC_H

#include <stddef.h>
#include <stdint.h>

#include "sm_alloc.h"
#include "sm_io.h"
#include "sm_proc_data.h"

typedef struct sm_ric_s sm_ric_t;

typedef struct {

  sm_subs_data_t (*on_subscription)(sm_ric_t const*, void* subs);

  sm_ag_if_rd_ind_t (*on_indication)(sm_ric_t const*, sm_ind_data_t const* data);

  sm_ctrl_req_data_t (*on_control_req)(sm_ric_t const*, void*);

  sm_ag_if_ans_ctrl_t (*on_control_out)(sm_ric_t const*, sm_ctrl_out_data_t const*);

  sm_ag_if_rd_e2setup_t (*on_e2_setup)(sm_ric_t const*, sm_e2_setup_data_t const*);

  sm_ag_if_rd_rsu_t (*on_ric_service_update)(sm_ric_t const*, sm_ric_service_update_data_t const*);

#ifdef E2AP_V3
  sm_ric_query_data_t (*on_ric_query)(sm_ric_t const*, void*);

  void (*on_subscription_mod)(sm_ric_t const* sm, void*);
#endif

} sm_e2ap_procedures_ric_t;

typedef struct sm_ric_s {

  // 5 Procedures stored at the SO
  sm_e2ap_procedures_ric_t proc; 

  // Free function
  void (*free_sm)(sm_ric_t* sm_ric);

  // (De)Allocation memory functions
  sm_alloc_t alloc;

  // Shared Object handle
  void* handle;

  // RAN Function ID
  uint16_t const ran_func_id;

  char ran_func_name[32];

} sm_ric_t;

#endif

