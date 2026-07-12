/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef E2_NODE_RIC_ID_H
#define E2_NODE_RIC_ID_H 

#include "../../lib/e2ap/e2ap_global_node_id_wrapper.h"
#include "../../lib/e2ap/ric_gen_id_wrapper.h"

typedef enum{
  SUBSCRIPTION_RIC_REQUEST_TYPE,
  CONTROL_RIC_REQUEST_TYPE,

  END_RIC_REQUEST_TYPE,
} ric_request_type_e;

typedef struct{
  global_e2_node_id_t e2_node_id;
  ric_gen_id_t ric_id;
  ric_request_type_e ric_req_type;
} e2_node_ric_id_t;

e2_node_ric_id_t cp_e2_node_ric_id(e2_node_ric_id_t const* src);

void free_e2_node_ric_id(e2_node_ric_id_t* src);

void  free_e2_node_ric_id_wrapper(void* src);

#endif
