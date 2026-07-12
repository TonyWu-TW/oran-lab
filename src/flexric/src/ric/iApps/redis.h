/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef REDIS_LISTENER_H
#define REDIS_LISTENER_H

#include "sm/agent_if/read/sm_ag_if_rd.h"

void notify_redis_listener(sm_ag_if_rd_ind_t const* data);

#endif


