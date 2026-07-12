/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef XAPP_MESSAGES_H
#define XAPP_MESSAGES_H 

#include <stddef.h>
#include <stdint.h>

typedef enum
{
  XAPP_MSG_KEEP_ALIVE,
  XAPP_MSG_REQUEST,
  XAPP_MSG_INIT,
  XAPP_MSG_UNKNOWN
} xapp_msg_type_e;

typedef struct
{
  int64_t dummy_data;
} xapp_init_msg_t;

typedef enum{
  XAPP_REQUEST_TYPE_REPORT,
  XAPP_REQUEST_TYPE_CONTROL,
  XAPP_REQUEST_TYPE_INSERT,
  XAPP_REQUEST_TYPE_POLICY,
  XAPP_REQUEST_TYPE_UNKNOWN,
} xapp_req_type_e; 

typedef enum{
  XAPP_MAC_REPORT,
  XAPP_RLC_REPORT,
  XAPP_PDCP_REPORT,
  XAPP_RRC_REPORT,
  XAPP_SLICE_REPORT,
} xapp_report_e; 

typedef enum{
  XAPP_ANS_OK,
  XAPP_ANS_UNKNOWN_ID,
  XAPP_ANS_ERROR,

  XAPP_ANS_NOT_IMPLEMENTED  
} xapp_request_answer_e;



typedef struct{
  uint32_t interval_ms;
} mac_report_t; 

typedef struct{
  uint32_t interval_ms;
} rlc_report_t; 

typedef struct{
  uint32_t interval_ms;
} rrc_report_t; 

typedef struct{
  uint32_t interval_ms;
} pdcp_report_t; 

typedef struct{
  uint32_t interval_ms;
} slice_report_t; 


typedef struct{

  xapp_report_e type;
  
  union{
    mac_report_t mac;   
    rlc_report_t rlc;
    pdcp_report_t pdcp;
    rrc_report_t rrc;
    slice_report_t slice;
  }; 

} xapp_request_report_t;

typedef struct
{


} xapp_request_control_t;

typedef struct
{


} xapp_request_insert_t;

typedef struct
{


} xapp_request_policy_t;

typedef struct
{
  uint32_t xapp_id;
  xapp_req_type_e type;

  union{
    xapp_request_report_t report;
    xapp_request_control_t control;
    xapp_request_insert_t insert;
    xapp_request_policy_t policy;
  };

} xapp_req_msg_t;

typedef struct
{
  int64_t xapp_id;
} xapp_keep_alive_msg_t;


typedef struct
{
  xapp_msg_type_e type;
  union{
    xapp_init_msg_t init;
    xapp_req_msg_t req;
    xapp_keep_alive_msg_t ping;
  };

} xapp_msg_t;


#endif

