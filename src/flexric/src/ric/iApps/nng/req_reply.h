/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef REQUEST_REPLY_SERVER
#define REQUEST_REPLY_SERVER 

#include "msgs/xapp_msgs.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
  char url[64];
  uint64_t (*init_fp)(xapp_init_msg_t const* msg, void* data);
  bool (*keep_alive_fp)(xapp_keep_alive_msg_t const*, void* data);
  xapp_request_answer_e (*request_fp)(xapp_req_msg_t const*, void* data);
  void* data;
} req_reply_server_arg_t;


void init_req_reply_server_ping(req_reply_server_arg_t* arg);

void init_req_reply_server_msg(req_reply_server_arg_t* arg);


void stop_req_reply_server_ping(void);

void stop_req_reply_server_msg(void);

#endif

