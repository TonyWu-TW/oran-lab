/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef KPM_SERVICE_MODEL_AGENT_H
#define KPM_SERVICE_MODEL_AGENT_H

#include "../../sm_agent.h"

/**
 * Allocation of ServiceModel KPM agent data structure using the Factory Pattern from Service Model generic code ()
 * Cfr: flexric/src/sm/sm_agent.h). This function will need to be called by the main program in the agent component of 
 * the client/server architecture when it wants to load the service Model KPM.
 * Adhering to spec 'SM KPM v.2.02'
 * 
 * @param[in] io structure containing defintion of read/write functions to communicate with the RAN
 * @return the agent structure just created
 */
__attribute__ ((visibility ("default"))) 
sm_agent_t *make_kpm_sm_agent(sm_io_ag_ran_t io);

#endif
