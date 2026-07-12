/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef MAC_DECODING_GENERIC
#define MAC_DECODING_GENERIC 

#include "kpm_dec_asn.h"
//#include "kpm_dec_fb.h"
// #include "kpm_dec_plain.h"

/////////////////////////////////////////////////////////////////////
// 9 Information Elements that are interpreted by the SM according
// to ORAN-WG3.E2SM-KPM v.2.0 Technical Specification
/////////////////////////////////////////////////////////////////////


#define kpm_dec_event_trigger(T,U,V) _Generic ((T), \
                           /* kpm_enc_plain_t*: kpm_dec_event_trigger_plain,*/  \
                           kpm_enc_asn_t*: kpm_dec_event_trigger_asn, \
                           default: kpm_dec_event_trigger_asn) (U,V)
                          
#define kpm_dec_action_def(T,U,V) _Generic ((T), \
                           /* kpm_enc_plain_t*: kpm_dec_action_def_plain,*/  \
                           kpm_enc_asn_t*: kpm_dec_action_def_asn, \
                           default: kpm_dec_action_def_asn) (U,V)

#define kpm_dec_ind_hdr(T,U,V) _Generic ((T), \
                           kpm_enc_asn_t*: kpm_dec_ind_hdr_asn, \
                           default:  kpm_dec_ind_hdr_asn) (U,V)

#define kpm_dec_ind_msg(T,U,V) _Generic ((T), \
                           kpm_enc_asn_t*: kpm_dec_ind_msg_asn, \
                           default:  kpm_dec_ind_msg_asn) (U,V)

#define kpm_dec_func_def(T,U,V) _Generic ((T), \
                           kpm_enc_asn_t*: kpm_dec_func_def_asn, \
                           default:  kpm_dec_func_def_asn) (U,V)
                           
#endif

