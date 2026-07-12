/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef MAC_SERVICE_MODEL_AGENT_H
#define MAC_SERVICE_MODEL_AGENT_H

#include <stddef.h>
#include <stdint.h>

#include "../sm_agent.h"

__attribute__ ((visibility ("default"))) 
sm_agent_t* make_mac_sm_agent(sm_io_ag_ran_t io);

#endif

