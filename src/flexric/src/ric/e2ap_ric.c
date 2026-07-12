/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "e2ap_ric.h"
#include "lib/e2ap/e2ap_msg_dec_generic_wrapper.h"  // for e2ap_msg_dec_gen
#include "lib/e2ap/e2ap_msg_enc_generic_wrapper.h"  // for e2ap_enc_control_reques...
#include "lib/e2ap/e2ap_msg_free_wrapper.h"        // for e2ap_free_control_request

#include <assert.h>                           // for assert
#include <stdio.h>                            // for NULL

void e2ap_msg_free_ric(e2ap_ric_t* ap, e2ap_msg_t* msg)
{
  assert(ap != NULL);
  assert(msg != NULL);
  const e2_msg_type_t msg_type = msg->type;   
  if(msg_type == NONE_E2_MSG_TYPE)
    return;

  ap->base.type.free_msg[msg_type](msg);
}

void e2ap_free_subscription_request_ric(e2ap_ric_t* ap, ric_subscription_request_t* sr)
{
  assert(ap != NULL);
  assert(sr != NULL);
  e2ap_free_subscription_request(sr);
}

void e2ap_free_control_request_ric(e2ap_ric_t* ap, ric_control_request_t* ctrl_req)
{
  assert(ap != NULL);
  assert(ctrl_req != NULL);
  e2ap_free_control_request(ctrl_req);
}


e2ap_msg_t e2ap_msg_dec_ric( e2ap_ric_t* ap, byte_array_t ba)
{
  assert(ap != NULL);
  e2ap_msg_t msg = e2ap_msg_dec_gen(&ap->base.type, ba);
  return msg;
}

byte_array_t e2ap_msg_enc_ric(e2ap_ric_t* ap, e2ap_msg_t* msg)
{
  assert(ap != NULL);
  assert(msg != NULL);
  const e2_msg_type_t msg_type = msg->type;   
  assert(msg_type < NONE_E2_MSG_TYPE);

  byte_array_t ba = ap->base.type.enc_msg[msg_type](msg);
  return ba;
}

byte_array_t e2ap_enc_subscription_request_ric(e2ap_ric_t* ap, ric_subscription_request_t const* sr)
{
  assert(ap != NULL);
  assert(sr != NULL);
  return e2ap_enc_subscription_request_gen(&ap->base.type, sr);
}

byte_array_t e2ap_enc_subscription_delete_request_ric(e2ap_ric_t* ap, ric_subscription_delete_request_t const* sd)
{
  assert(ap != NULL);
  assert(sd != NULL);
  return e2ap_enc_subscription_delete_request_gen(&ap->base.type, sd);
}

byte_array_t e2ap_enc_setup_response_ric(e2ap_ric_t* ap, e2_setup_response_t* sr)
{
  assert(ap != NULL);
  assert(sr != NULL);
  return e2ap_enc_setup_response_gen(&ap->base.type, sr);
}

byte_array_t e2ap_enc_control_request_ric(e2ap_ric_t* ap, ric_control_request_t const* ctrl_req)
{
  assert(ap != NULL);
  assert(ctrl_req != NULL);
  return e2ap_enc_control_request_gen(&ap->base.type,ctrl_req);
}

