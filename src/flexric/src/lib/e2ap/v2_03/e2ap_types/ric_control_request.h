/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef RIC_CONTROL_REQUEST_H
#define RIC_CONTROL_REQUEST_H

#include "util/byte_array.h"
#include "common/ric_gen_id.h"
#include <stdbool.h>

typedef enum { 
  RIC_CONTROL_REQUEST_NO_ACK = 0,
  RIC_CONTROL_REQUEST_ACK = 1, 
  RIC_CONTROL_REQUEST_NACK = 2
} ric_control_ack_req_t;

typedef struct {
  ric_gen_id_t ric_id;
  byte_array_t* call_process_id; // optional
  byte_array_t hdr;
  byte_array_t msg;
  ric_control_ack_req_t* ack_req; // optional
} ric_control_request_t;

bool eq_ric_control_request(const ric_control_request_t* m0, const ric_control_request_t* m1);

#endif

