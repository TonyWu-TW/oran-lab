/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E2AP_XAPP_H
#define E2AP_XAPP_H

#include "lib/e2ap/e2ap_ap_wrapper.h"                                     // for e2ap_...
#include "lib/e2ap/e2_setup_response_wrapper.h"                // for e2_se...
#include "lib/e2ap/e42_setup_request_wrapper.h"                
#include "lib/e2ap/e42_ric_subscription_request_wrapper.h"                
#include "lib/e2ap/e42_ric_control_request_wrapper.h"

#include "lib/e2ap/ric_control_request_wrapper.h"              // for ric_c...
#include "lib/e2ap/ric_subscription_delete_request_wrapper.h"  // for ric_s...
#include "lib/e2ap/ric_subscription_request_wrapper.h"         // for ric_s...
#include "lib/e2ap/type_defs_wrapper.h"                           // for e2ap_...
#include "util/byte_array.h"                                    // for byte_...


typedef struct
{
  e2ap_ap_t base;
} e2ap_xapp_t;


/////////
// Free
/////////

void e2ap_msg_free_xapp(e2ap_xapp_t* xapp, e2ap_msg_t* msg);

void e2ap_free_subscription_request_xapp(e2ap_xapp_t* ap, ric_subscription_request_t* sr);  

void e2ap_free_control_request_xapp(e2ap_xapp_t* ap, ric_control_request_t* ctrl_req);

//////////////
// Encoding
//////////////

byte_array_t e2ap_msg_enc_xapp(e2ap_xapp_t* ap, e2ap_msg_t* msg);

//byte_array_t e2ap_enc_setup_request_xapp(e2ap_xapp_t* ap, e2_setup_request_t* sr);

byte_array_t e2ap_enc_e42_setup_request_xapp(e2ap_xapp_t* ap, e42_setup_request_t* sr);

byte_array_t e2ap_enc_subscription_request_xapp(e2ap_xapp_t* ap, ric_subscription_request_t* sr);

byte_array_t e2ap_enc_subscription_delete_request_xapp(e2ap_xapp_t* ap, ric_subscription_delete_request_t* sd);

byte_array_t e2ap_enc_control_request_xapp(e2ap_xapp_t* ap, ric_control_request_t* ctrl_req);

byte_array_t e2ap_enc_e42_setup_request_xapp(e2ap_xapp_t* ap, e42_setup_request_t* sr);

byte_array_t e2ap_enc_e42_subscription_request_xapp(e2ap_xapp_t* ap, e42_ric_subscription_request_t* sr);

byte_array_t e2ap_enc_ric_subscription_delete_xapp(e2ap_xapp_t* ap, ric_subscription_delete_request_t* sdr);

byte_array_t e2ap_enc_e42_ric_subscription_delete_xapp(e2ap_xapp_t* ap, e42_ric_subscription_delete_request_t* sdr);

byte_array_t e2ap_enc_e42_control_request_xapp(e2ap_xapp_t* ap, e42_ric_control_request_t* cr);

//////////////
// Decoding
//////////////

e2ap_msg_t e2ap_msg_dec_xapp( e2ap_xapp_t* ap, byte_array_t ba);


#endif

