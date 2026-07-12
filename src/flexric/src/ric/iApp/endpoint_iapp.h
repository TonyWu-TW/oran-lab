/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E2AP_ENDPOINT_IAPP_H
#define E2AP_ENDPOINT_IAPP_H

#include "lib/ep/e2ap_ep.h"   // for e2ap_ep_t
#include "util/byte_array.h"  // for byte_array_t
#include "map_xapps_sockaddr.h"

typedef struct{
  e2ap_ep_t base;

  // Multi-connection supported
  // xApp ID -> sctp_info_t  
  map_xapps_sockaddr_t xapps; 

} e2ap_ep_iapp_t;

void e2ap_init_ep_iapp(e2ap_ep_iapp_t* ep, const char* addr, int port);

void e2ap_free_ep_iapp(e2ap_ep_iapp_t* ep);

sctp_msg_t e2ap_recv_msg_iapp(e2ap_ep_iapp_t* ep);

//void e2ap_send_bytes_iapp(const e2ap_ep_iapp_t* ep, byte_array_t ba);
void e2ap_send_bytes_iapp(const e2ap_ep_iapp_t* ep, int xapp_id, byte_array_t ba);

void e2ap_send_sctp_msg_iapp(const e2ap_ep_iapp_t* ep, sctp_msg_t* msg);

void e2ap_reg_sock_addr_iapp(e2ap_ep_iapp_t* ep, uint16_t xapp_id, sctp_info_t* s);

#endif

