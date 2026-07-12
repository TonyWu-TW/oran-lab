/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "fill_rnd_data_gtp.h"
#include "../../src/util/time_now_us.h"

#include <assert.h>
#include <stdlib.h>
#include <time.h>

void fill_gtp_ind_data(gtp_ind_data_t* ind)
{
  assert(ind != NULL);

  srand(time(0));

  int const mod = 1024;

  // Get indication message
  gtp_ind_msg_t* ind_msg = &ind->msg;
  
  // Set time now  
  ind_msg->tstamp = time_now_us();

  // Set random number of messages  
  ind_msg->len = rand()%4;
  if(ind_msg->len > 0 ){  
    ind_msg->ngut = calloc(ind_msg->len, sizeof(gtp_ngu_t_stats_t) );
    assert(ind_msg->ngut != NULL);
  }
    
  for(uint32_t i = 0; i < ind_msg->len; ++i){
    gtp_ngu_t_stats_t* ngut = &ind_msg->ngut[i];
      
    // Fill dummy data in your data structure  
    ngut->rnti=abs(rand()%mod) ;         
    ngut->qfi=abs(rand()%mod);
    ngut->teidgnb=abs(rand()%mod);
    ngut->teidupf=abs(rand()%mod);
  }
}

