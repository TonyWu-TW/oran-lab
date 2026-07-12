/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#include "e2ap_iapp.h"

#include <assert.h>
#include <stdlib.h>

#include "lib/e2ap/type_defs_wrapper.h"
#include "lib/e2ap/e2ap_msg_enc_generic_wrapper.h"
#include "lib/e2ap/e2ap_msg_dec_generic_wrapper.h"
#include "lib/e2ap/e2ap_msg_free_wrapper.h"

void e2ap_msg_free_iapp(e2ap_iapp_t* ap, e2ap_msg_t* msg)
{
  assert(ap != NULL);
  assert(msg != NULL);
  e2_msg_type_t const msg_type = msg->type;   
  if(msg_type == NONE_E2_MSG_TYPE) // Nothing to free
    return;
  assert(msg_type < NONE_E2_MSG_TYPE);
  ap->base.type.free_msg[msg_type](msg);
}


//
// Encoding
//


byte_array_t e2ap_msg_enc_iapp(e2ap_iapp_t* ap, const e2ap_msg_t* msg)
{
  assert(ap != NULL);
  assert(msg != NULL);
  e2_msg_type_t const msg_type = msg->type ;
  assert(msg_type < NONE_E2_MSG_TYPE);
  return ap->base.type.enc_msg[msg_type](msg);
}

byte_array_t e2ap_enc_setup_response_iapp(e2ap_iapp_t* ap, e2_setup_response_t* sr)
{
  assert(ap != NULL);
  assert(sr != NULL);
  return e2ap_enc_setup_response_gen(&ap->base.type, sr);
}


byte_array_t e2ap_enc_subscription_response_iapp(e2ap_iapp_t* ap, const ric_subscription_response_t* sr)
{
  assert(ap != NULL);
  assert(sr != NULL);
  return e2ap_enc_subscription_response_gen(&ap->base.type, sr);
}

byte_array_t e2ap_enc_subscription_failure_iapp(e2ap_iapp_t* ap,const ric_subscription_failure_t* sf)
{
  assert(ap != NULL);
  assert(sf != NULL);
  return e2ap_enc_subscription_failure_gen(&ap->base.type,sf);
}

byte_array_t e2ap_enc_indication_iapp(e2ap_iapp_t* ap, const ric_indication_t* ind)
{
  assert(ap != NULL);
  assert(ind != NULL);
  return e2ap_enc_indication_gen(&ap->base.type, ind);
}

byte_array_t e2ap_enc_subscription_delete_response_iapp(e2ap_iapp_t* ap, const ric_subscription_delete_response_t*  sdr)
{
  assert(ap != NULL);
  assert(sdr != NULL);
  return e2ap_enc_subscription_delete_response_gen(&ap->base.type, sdr);
}

byte_array_t e2ap_enc_subscription_delete_failure_iapp(e2ap_iapp_t* ap, const ric_subscription_delete_failure_t*  sdf)
{
  assert(ap != NULL);
  assert(sdf != NULL);
  return e2ap_enc_subscription_delete_failure_gen(&ap->base.type, sdf);
}

byte_array_t e2ap_enc_control_acknowledge_iapp(e2ap_iapp_t* ap, const ric_control_acknowledge_t* ca)
{
  assert(ap != NULL);
  assert(ca != NULL);
  return  e2ap_enc_control_acknowledge_gen(&ap->base.type, ca);
}

byte_array_t e2ap_enc_control_failure_iapp(e2ap_iapp_t* ap, const ric_control_failure_t* cf)
{
  assert(ap != NULL);
  assert(cf != NULL);
  return  e2ap_enc_control_failure_gen(&ap->base.type, cf);
}

 //
 // Decoding
 //
 
e2ap_msg_t e2ap_msg_dec_iapp(e2ap_iapp_t* ap, byte_array_t ba)
{
  assert(ap != NULL);
  assert(ba.buf != NULL);
  return e2ap_msg_dec_gen(&ap->base.type, ba);
}

