/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef PDCP_SERVICE_MODEL_AGENT_H
#define PDCP_SERVICE_MODEL_AGENT_H

#include "../sm_agent.h"

__attribute__ ((visibility ("default"))) 
sm_agent_t* make_pdcp_sm_agent(sm_io_ag_ran_t io);

#endif

