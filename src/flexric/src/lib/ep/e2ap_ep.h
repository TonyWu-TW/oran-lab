/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef E2AP_EP
#define E2AP_EP

#include <assert.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h> 

#include "util/byte_array.h"
#include "sctp_msg.h"


typedef struct{
  const char addr[16]; // only ipv4 supported
  const int port;
  const int fd;
  pthread_mutex_t mtx;
} e2ap_ep_t;

void e2ap_ep_init(e2ap_ep_t* ep);

void e2ap_ep_free(e2ap_ep_t* ep);

void e2ap_send_sctp_msg(const e2ap_ep_t* ep, sctp_msg_t* msg);

sctp_msg_t e2ap_recv_sctp_msg(e2ap_ep_t* ep);

#endif

