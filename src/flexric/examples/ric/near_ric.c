/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#include "../../src/ric/near_ric_api.h"
#include "../../src/lib/sig_handler.h"

#include <arpa/inet.h>
#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <poll.h>
#include <time.h>
#include <unistd.h>

const uint16_t MAC_ran_func_id = 142;
const uint16_t RLC_ran_func_id = 143;
const uint16_t PDCP_ran_func_id = 144;
const uint16_t SLICE_ran_func_id = 145; // Not implemented yet
const uint16_t KPM_ran_func_id = 147;
const char* cmd = "5_ms";

int main(int argc, char *argv[])
{
  init_sig_handler();

  fr_args_t args = init_fr_args(argc, argv);
 
  // Init the RIC
  init_near_ric_api(&args);

  poll_and_wait_sig();

  stop_near_ric_api();
  printf("The nearRT-RIC run SUCCESSFULLY\n");

  return EXIT_SUCCESS;
}

