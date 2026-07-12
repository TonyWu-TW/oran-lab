/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef E2AP_IAPP_H
#define E2AP_IAPP_H

#include "lib/e2ap/e2ap_ap_wrapper.h"                                      // for e2ap...
#include "lib/e2ap/e2_setup_request_wrapper.h"                  // for e2_s...
#include "lib/e2ap/ric_control_ack_wrapper.h"                   // for ric_...
#include "lib/e2ap/ric_control_failure_wrapper.h"               // for ric_...
#include "lib/e2ap/ric_indication_wrapper.h"                    // for ric_...
#include "lib/e2ap/ric_subscription_delete_failure_wrapper.h"   // for ric_...
#include "lib/e2ap/ric_subscription_delete_response_wrapper.h"  // for ric_...
#include "lib/e2ap/ric_subscription_failure_wrapper.h"          // for ric_...
#include "lib/e2ap/ric_subscription_response_wrapper.h"         // for ric_...
#include "lib/e2ap/type_defs_wrapper.h"                                    // for e2ap...
                                                                 //
#include "util/byte_array.h"                                     // for byte...

typedef struct e2ap_iapp_s 
{
  e2ap_ap_t base;

} e2ap_iapp_t;

void e2ap_msg_free_iapp(e2ap_iapp_t* ag, e2ap_msg_t* msg);

/////////////
// Encoding
//////////////

byte_array_t e2ap_enc_setup_response_iapp(e2ap_iapp_t* ap, e2_setup_response_t* sr);


byte_array_t e2ap_msg_enc_iapp(e2ap_iapp_t* ap, const e2ap_msg_t* msg); 

byte_array_t e2ap_enc_subscription_response_iapp(e2ap_iapp_t* ap, const ric_subscription_response_t* sr);

byte_array_t e2ap_enc_subscription_failure_iapp(e2ap_iapp_t* ap,const ric_subscription_failure_t* sf);

byte_array_t e2ap_enc_indication_iapp(e2ap_iapp_t* ap, const ric_indication_t* ind);

byte_array_t e2ap_enc_subscription_delete_response_iapp(e2ap_iapp_t* ap, const ric_subscription_delete_response_t*  sdr);

byte_array_t e2ap_enc_subscription_delete_failure_iapp(e2ap_iapp_t* ap, const ric_subscription_delete_failure_t*  sdf);

byte_array_t e2ap_enc_control_acknowledge_iapp(e2ap_iapp_t* ap, const ric_control_acknowledge_t* ca);

byte_array_t e2ap_enc_control_failure_iapp(e2ap_iapp_t* ap, const ric_control_failure_t* cf);


//
// Decoding
//

e2ap_msg_t e2ap_msg_dec_iapp(e2ap_iapp_t* ap, byte_array_t ba); 

#endif

