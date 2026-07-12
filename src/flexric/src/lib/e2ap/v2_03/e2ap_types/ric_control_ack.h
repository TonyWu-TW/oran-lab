/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef RIC_CONTROL_ACK_H
#define RIC_CONTROL_ACK_H

#include "common/ric_gen_id.h"
#include "util/byte_array.h"

typedef struct {
  ric_gen_id_t ric_id;
  byte_array_t* call_process_id; // optional
  byte_array_t* control_outcome; // optional
} ric_control_acknowledge_t;

bool eq_ric_control_ack_req(const ric_control_acknowledge_t* m0, const ric_control_acknowledge_t* m1);

#endif

