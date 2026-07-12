/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef ENDPOINT_XAPP
#define ENDPOINT_XAPP

#include "lib/ep/e2ap_ep.h"   // for e2ap_ep_t
#include "util/byte_array.h"  // for byte_array_t

typedef struct e2ap_xapp_xapp
{
  e2ap_ep_t base;

  // Only one connection supported 
  struct sockaddr_in to; 
  struct sctp_sndrcvinfo sri;
  int msg_flags;
} e2ap_ep_xapp_t;

void e2ap_init_ep_xapp(e2ap_ep_xapp_t* ep, const char* addr, int port);

void e2ap_free_ep_xapp(e2ap_ep_xapp_t* ep);

byte_array_t e2ap_recv_msg_xapp(e2ap_ep_xapp_t* ep);

void e2ap_send_bytes_xapp(e2ap_ep_xapp_t* ep, byte_array_t ba);

#endif

