/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef E2AP_APPLICATION_PROTOCOL
#define E2AP_APPLICATION_PROTOCOL

#include <assert.h>
#include "type_defs.h"

#include "e2ap_ap_asn.h"
#include "e2ap_ap_fb.h"

struct E2AP_PDU;
typedef e2ap_msg_t (*e2ap_gen_dec_asn_fp)(const struct E2AP_PDU*);
typedef byte_array_t (*e2ap_gen_enc_asn_fp)(const e2ap_msg_t*);

struct e2ap_E2Message_table;
typedef e2ap_msg_t (*e2ap_gen_dec_fb_fp)(const struct e2ap_E2Message_table*);
typedef byte_array_t (*e2ap_gen_enc_fb_fp)(const e2ap_msg_t*);


typedef void (*e2ap_free_fp)(e2ap_msg_t*);

typedef struct e2ap_asn{
  e2ap_gen_enc_asn_fp enc_msg[31];
  e2ap_gen_dec_asn_fp dec_msg[31];
  e2ap_free_fp free_msg[31];
} e2ap_asn_t;

typedef struct e2ap_fb{
  e2ap_gen_enc_fb_fp enc_msg[31];
  e2ap_gen_dec_fb_fp dec_msg[31];
  e2ap_free_fp free_msg[31];
} e2ap_fb_t;


typedef struct{
#ifdef ASN
 e2ap_asn_t type;
#elif FLATBUFFERS
 e2ap_fb_t type;
#else
  static_assert(0!=0, "Error. No encoding scheme selected");
#endif

} e2ap_ap_t;


#define init_ap(T) _Generic ((T),   e2ap_asn_t*:  init_ap_asn, \
                                    e2ap_fb_t*:   init_ap_fb, \
                                    default:      init_ap_asn) (T)

#endif

