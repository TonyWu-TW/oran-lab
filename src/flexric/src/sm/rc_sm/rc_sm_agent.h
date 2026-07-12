/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef RC_SERVICE_MODEL_AGENT_H
#define RC_SERVICE_MODEL_AGENT_H

#include <stddef.h>
#include <stdint.h>
#include "../sm_agent.h"

__attribute__ ((visibility ("default"))) 
sm_agent_t* make_rc_sm_agent(sm_io_ag_ran_t io);

#endif

