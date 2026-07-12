/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef E2_AGENT_API_MOSAIC_H
#define E2_AGENT_API_MOSAIC_H

#include "../sm/sm_io.h"
#include "../util/conf_file.h"
#include "../util/ngran_types.h"

void init_agent_api(int mcc, 
                    int mnc, 
                    int mnc_digit_len,
                    int nb_id,
                    int cu_du_id,
                    ngran_node_t ran_type,
                    sm_io_ag_ran_t io,
                    fr_args_t const* args);

void stop_agent_api(void);

void async_event_agent_api(uint32_t ric_req_id, void* ind_data);

#endif

