/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E2AP_MSG_DEC_GENERIC_H
#define E2AP_MSG_DEC_GENERIC_H

#include "e2ap_msg_dec_asn.h" 
#include "e2ap_msg_dec_fb.h" 


#define e2ap_msg_dec_gen(T,U) _Generic ((T),  e2ap_asn_t*: e2ap_msg_dec_asn, \
                                          e2ap_fb_t*: e2ap_msg_dec_fb, \
                                          default: e2ap_msg_dec_asn) (T,U)





#endif


