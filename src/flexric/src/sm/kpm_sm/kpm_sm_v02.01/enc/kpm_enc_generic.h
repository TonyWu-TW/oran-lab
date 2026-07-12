/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/*
 * implementation via 'static' (that is, decided at compile time) polymorphism of the encoding part
 */
#ifndef KPM_ENCODE_GENERIC_V2
#define KPM_ENCODE_GENERIC_V2

#include "kpm_enc_asn.h"
// #include "kpm_enc_plain.h"

/////////////////////////////////////////////////////////////////////
// 9 Information Elements that are interpreted by the SM according
// to ORAN-WG3.E2SM-v01.00.00 Technical Specification
/////////////////////////////////////////////////////////////////////

#define kpm_enc_event_trigger(T,U) _Generic ((T), \
                           /* kpm_enc_plain_t*: kpm_enc_event_trigger_plain, */ \
                           kpm_enc_asn_t*: kpm_enc_event_trigger_asn,\
                           default: kpm_enc_event_trigger_asn) (U)

#define kpm_enc_action_def(T,U) _Generic ((T), \
                           kpm_enc_asn_t*: kpm_enc_action_def_asn, \
                           default:  kpm_enc_action_def_asn) (U)

#define kpm_enc_ind_hdr(T,U) _Generic ((T), \
                           kpm_enc_asn_t*: kpm_enc_ind_hdr_asn, \
                           default:  kpm_enc_ind_hdr_asn) (U)

#define kpm_enc_ind_msg(T,U) _Generic ((T), \
                           kpm_enc_asn_t*: kpm_enc_ind_msg_asn, \
                           default:  kpm_enc_ind_msg_asn) (U)

#define kpm_enc_func_def(T,U) _Generic ((T), \
                           kpm_enc_asn_t*: kpm_enc_func_def_asn, \
                           default:  kpm_enc_func_def_asn) (U)

#endif
