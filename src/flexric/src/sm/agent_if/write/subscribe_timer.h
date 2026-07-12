/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef SUBSCRIBE_TIMER_EUR_H
#define SUBSCRIBE_TIMER_EUR_H

#include "../../kpm_sm/kpm_data_ie_wrapper.h"
#include <stdint.h>
#include <stdlib.h>

typedef enum{
  KPM_V3_0_SUB_DATA_ENUM,
  RAN_CONTROL_V1_3_SUB_DATA_ENUM,
  NONE_SUB_DATA_ENUM,

  END_SUB_DATA_ENUM,
} sub_data_e;

typedef struct{
  int64_t ms;
  sub_data_e type;
  // Number of elements.
  // Just one is supported
  size_t sz;
  void* act_def; // e.g., kpm_act_def_t
} subscribe_timer_t;

void free_subscribe_timer(subscribe_timer_t* src);

#endif

