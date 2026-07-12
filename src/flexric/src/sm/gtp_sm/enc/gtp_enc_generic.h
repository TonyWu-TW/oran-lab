/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef GTP_ENCODING_GENERIC
#define GTP_ENCODING_GENERIC 

#include "gtp_enc_plain.h"

/////////////////////////////////////////////////////////////////////
// 9 Information Elements that are interpreted by the SM according
// to ORAN-WG3.E2SM-v01.00.00 Technical Specification
/////////////////////////////////////////////////////////////////////


#define gtp_enc_event_trigger(T,U) _Generic ((T), \
                           gtp_enc_plain_t*: gtp_enc_event_trigger_plain, \
                           default: gtp_enc_event_trigger_plain) (U)

#define gtp_enc_action_def(T,U) _Generic ((T), \
                           gtp_enc_plain_t*: gtp_enc_action_def_plain, \
                           default:  gtp_enc_action_def_plain) (U)

#define gtp_enc_ind_hdr(T,U) _Generic ((T), \
                           gtp_enc_plain_t*: gtp_enc_ind_hdr_plain , \
                           default:  gtp_enc_ind_hdr_plain) (U)

#define gtp_enc_ind_msg(T,U) _Generic ((T), \
                           gtp_enc_plain_t*: gtp_enc_ind_msg_plain , \
                           default:  gtp_enc_ind_msg_plain) (U)

#define gtp_enc_call_proc_id(T,U) _Generic ((T), \
                           gtp_enc_plain_t*: gtp_enc_call_proc_id_plain , \
                           default:  gtp_enc_call_proc_id_plain) (U)

#define gtp_enc_ctrl_hdr(T,U) _Generic ((T), \
                           gtp_enc_plain_t*: gtp_enc_ctrl_hdr_plain , \
                           default:  gtp_enc_ctrl_hdr_plain) (U)

#define gtp_enc_ctrl_msg(T,U) _Generic ((T), \
                           gtp_enc_plain_t*: gtp_enc_ctrl_msg_plain , \
                           default:  gtp_enc_ctrl_msg_plain) (U)

#define gtp_enc_ctrl_out(T,U) _Generic ((T), \
                           gtp_enc_plain_t*: gtp_enc_ctrl_out_plain , \
                           default:  gtp_enc_ctrl_out_plain) (U)

#define gtp_enc_func_def(T,U) _Generic ((T), \
                           gtp_enc_plain_t*: gtp_enc_func_def_plain, \
                           default:  gtp_enc_func_def_plain) (U)

#endif

