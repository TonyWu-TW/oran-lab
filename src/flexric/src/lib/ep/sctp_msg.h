/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef SCTP_MSG_H
#define SCTP_MSG_H

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

#include <stdbool.h>
#include "util/byte_array.h"

typedef enum {
  SCTP_MSG_PAYLOAD,
  SCTP_MSG_NOTIFICATION
} sctp_msg_type_t;

typedef struct{
  struct sockaddr_in addr; 
  struct sctp_sndrcvinfo sri;
} sctp_info_t ;

int cmp_sctp_info_wrapper(void const* m0, void const* m1);

bool eq_sctp_info_wrapper(void const* m0, void const* m1);

typedef struct{
  sctp_msg_type_t type;
  sctp_info_t info;
  union{
    byte_array_t ba;
    union sctp_notification* notif;
  };
} sctp_msg_t;

void free_sctp_msg(sctp_msg_t* rcv);

#endif

