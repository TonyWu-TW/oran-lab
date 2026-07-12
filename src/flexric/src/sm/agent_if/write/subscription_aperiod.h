/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef SUBSCRIPTION_APERIODIC_MIR_H
#define SUBSCRIPTION_APERIODIC_MIR_H 

#include <stdint.h>

typedef struct {
  void (*free_aper_subs)(uint32_t ric_req_id);
} susbcription_aperiod_t;

#endif
