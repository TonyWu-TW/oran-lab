/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef RIC_SERVICE_UPDATE_ACK_H
#define RIC_SERVICE_UPDATE_ACK_H 

#include <stddef.h>
#include <stdint.h>
#include "common/e2ap_rejected_ran_function.h"
#include "common/e2ap_ran_function_id.h"

typedef struct {
  uint8_t trans_id;
  ran_function_id_t* accepted;
  size_t len_accepted;
  rejected_ran_function_t* rejected;
  size_t len_rejected;
} ric_service_update_ack_t;

bool eq_ric_service_update_ack(const  ric_service_update_ack_t* m0, const  ric_service_update_ack_t* m1);

#endif

