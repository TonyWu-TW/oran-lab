/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#include "redis.h"

#include <assert.h>
#include <stdio.h>

void notify_redis_listener(sm_ag_if_rd_ind_t const* data)
{
  assert(data != NULL);
  assert(data->type == MAC_STATS_V0 || data->type == RLC_STATS_V0 || data->type == PDCP_STATS_V0 
      || data->type == SLICE_STATS_V0 || data->type == KPM_STATS_V3_0 || data->type == GTP_STATS_V0
      || data->type == RAN_CTRL_STATS_V1_03 || data->type == TC_STATS_V0  ); 
 
  /*
  if(data->type == MAC_STATS_V0)
    printf("REDIS data called from MAC stats!!\n");
  else if (data->type == RLC_STATS_V0 )
    printf("REDIS data called from RLC stats!!\n");
  else
    assert(0!=0 && "Invalid data path");
    */
}
