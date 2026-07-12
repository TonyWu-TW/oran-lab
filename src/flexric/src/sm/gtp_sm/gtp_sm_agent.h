/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef GTP_SERVICE_MODEL_AGENT_H
#define GTP_SERVICE_MODEL_AGENT_H

#include "../sm_agent.h"
#include <stddef.h>
#include <stdint.h>

__attribute__ ((visibility ("default"))) 
sm_agent_t* make_gtp_sm_agent(sm_io_ag_ran_t io);

#endif

