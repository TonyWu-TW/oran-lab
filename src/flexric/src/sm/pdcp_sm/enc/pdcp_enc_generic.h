/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef PDCP_ENCODING_GENERIC
#define PDCP_ENCODING_GENERIC 

#include "pdcp_enc_asn.h"
#include "pdcp_enc_fb.h"
#include "pdcp_enc_plain.h"

/////////////////////////////////////////////////////////////////////
// 9 Information Elements that are interpreted by the SM according
// to ORAN-WG3.E2SM-v01.00.00 Technical Specification
/////////////////////////////////////////////////////////////////////


#define pdcp_enc_event_trigger(T,U) _Generic ((T), \
                           pdcp_enc_plain_t*: pdcp_enc_event_trigger_plain, \
                           pdcp_enc_asn_t*: pdcp_enc_event_trigger_asn,\
                           pdcp_enc_fb_t*: pdcp_enc_event_trigger_fb,\
                           default: pdcp_enc_event_trigger_plain) (U)

#define pdcp_enc_action_def(T,U) _Generic ((T), \
                           pdcp_enc_plain_t*: pdcp_enc_action_def_plain, \
                           pdcp_enc_asn_t*: pdcp_enc_action_def_asn, \
                           pdcp_enc_fb_t*: pdcp_enc_action_def_fb, \
                           default:  pdcp_enc_action_def_plain) (U)

#define pdcp_enc_ind_hdr(T,U) _Generic ((T), \
                           pdcp_enc_plain_t*: pdcp_enc_ind_hdr_plain , \
                           pdcp_enc_asn_t*: pdcp_enc_ind_hdr_asn, \
                           pdcp_enc_fb_t*: pdcp_enc_ind_hdr_fb, \
                           default:  pdcp_enc_ind_hdr_plain) (U)

#define pdcp_enc_ind_msg(T,U) _Generic ((T), \
                           pdcp_enc_plain_t*: pdcp_enc_ind_msg_plain , \
                           pdcp_enc_asn_t*: pdcp_enc_ind_msg_asn, \
                           pdcp_enc_fb_t*: pdcp_enc_ind_msg_fb, \
                           default:  pdcp_enc_ind_msg_plain) (U)

#define pdcp_enc_call_proc_id(T,U) _Generic ((T), \
                           pdcp_enc_plain_t*: pdcp_enc_call_proc_id_plain , \
                           pdcp_enc_asn_t*: pdcp_enc_call_proc_id_asn, \
                           pdcp_enc_fb_t*: pdcp_enc_call_proc_id_fb, \
                           default:  pdcp_enc_call_proc_id_plain) (U)

#define pdcp_enc_ctrl_hdr(T,U) _Generic ((T), \
                           pdcp_enc_plain_t*: pdcp_enc_ctrl_hdr_plain , \
                           pdcp_enc_asn_t*: pdcp_enc_ctrl_hdr_asn, \
                           pdcp_enc_fb_t*: pdcp_enc_ctrl_hdr_fb, \
                           default:  pdcp_enc_ctrl_hdr_plain) (U)

#define pdcp_enc_ctrl_msg(T,U) _Generic ((T), \
                           pdcp_enc_plain_t*: pdcp_enc_ctrl_msg_plain , \
                           pdcp_enc_asn_t*: pdcp_enc_ctrl_msg_asn, \
                           pdcp_enc_fb_t*: pdcp_enc_ctrl_msg_fb, \
                           default:  pdcp_enc_ctrl_msg_plain) (U)

#define pdcp_enc_ctrl_out(T,U) _Generic ((T), \
                           pdcp_enc_plain_t*: pdcp_enc_ctrl_out_plain , \
                           pdcp_enc_asn_t*: pdcp_enc_ctrl_out_asn, \
                           pdcp_enc_fb_t*: pdcp_enc_ctrl_out_fb, \
                           default:  pdcp_enc_ctrl_out_plain) (U)

#define pdcp_enc_func_def(T,U) _Generic ((T), \
                           pdcp_enc_plain_t*: pdcp_enc_func_def_plain, \
                           pdcp_enc_asn_t*: pdcp_enc_func_def_asn, \
                           pdcp_enc_fb_t*:  pdcp_enc_func_def_fb, \
                           default:  pdcp_enc_func_def_plain) (U)

#endif

