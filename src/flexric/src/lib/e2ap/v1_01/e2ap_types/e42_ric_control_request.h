/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef E42_RIC_CONTROL_REQUEST_H
#define E42_RIC_CONTROL_REQUEST_H 

#include <stdint.h>

#include "common/e2ap_global_node_id.h"
#include "ric_control_request.h"

typedef struct{
  uint16_t xapp_id; 
  global_e2_node_id_t id;
  ric_control_request_t ctrl_req;
} e42_ric_control_request_t;


bool eq_e42_ric_control_request(const e42_ric_control_request_t* m0, const  e42_ric_control_request_t* m1);


#endif

