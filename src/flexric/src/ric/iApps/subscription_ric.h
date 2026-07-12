/*
 *
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef SUBSCRIPTION_RIC_H
#define SUBSCRIPTION_RIC_H

#include "../../sm/agent_if/read/sm_ag_if_rd.h"

typedef struct{
  char name[32];
  void (*fp)(sm_ag_if_rd_ind_t const* data);
} subs_ric_t;



#endif

