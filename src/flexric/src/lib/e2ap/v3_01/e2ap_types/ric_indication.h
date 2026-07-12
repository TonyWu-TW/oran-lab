/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef RIC_INDICATION_H
#define RIC_INDICATION_H

#include "common/ric_gen_id.h"
#include "util/byte_array.h"
#include <stdint.h> 

typedef enum {
  RIC_IND_REPORT = 0,
  RIC_IND_INSERT =1,
} ric_indication_type_e;

typedef struct ric_indication {
  ric_gen_id_t ric_id;
  uint8_t action_id;
  uint16_t* sn; // optional
  ric_indication_type_e type;
  byte_array_t hdr;
  byte_array_t msg;
  byte_array_t* call_process_id; // optional
} ric_indication_t;

bool eq_ric_indication(const ric_indication_t* m0, const ric_indication_t* m1);

ric_indication_t mv_ric_indication(ric_indication_t* ind);

#endif

