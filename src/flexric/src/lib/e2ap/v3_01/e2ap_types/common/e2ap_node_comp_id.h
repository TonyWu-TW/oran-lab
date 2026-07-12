/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef E2AP_NODE_COMP_ID_MIR_H
#define E2AP_NODE_COMP_ID_MIR_H 

#include "e2ap_node_comp_interface_type.h"
#include "../../../../3gpp/ie/global_enb_id.h"
#include "../../../../3gpp/ie/global_ng_ran_node_id.h"
#include "../../../../3gpp/ie/global_en_gnb_id.h"
#include "../../../../3gpp/ie/s1_setup_response.h"
#include "../../../../../util/byte_array.h"

typedef struct{
  // 3GPP 36.423 [25] clause 9.2.22
  global_enb_id_t global_enb_id;
  // 3GPP 36.423 [25] clause 9.2.112
  global_en_gnb_id_t* global_en_gnb_id; 
} x2_global_enb_t;

// 9.2.32
typedef struct{
  e2ap_node_comp_interface_type_e type;
  union{
    // 3GPP 38.413 [19] clause 9.3.3.21 
    //[1-150]
    byte_array_t ng_amf_name;

    // 3GPP 38.423 [20] clause 9.2.2.3  
    global_ng_ran_node_id_t  xn_global_ng_ran_node_id;

    // 3GPP 38.463 [21] clause 9.3.1.15
    // INTEGER
    // (0 .. 2^36-1)
    uint64_t e1_gnb_cu_up_id;

    // 3GPP 38.473 [22] clause 9.3.1.9
    // INTEGER
    // (0 .. 2^36-1) 
    uint64_t f1_gnb_du_id; 

    // 3GPP 37.473 [23] clause
    // WARNING: missing clause in standard. Supposing 9.3.1.9
    // INTEGER
    // (0 .. 2^36-1) 
    uint64_t w1_ng_enb_du_id;

    // 3GPP 36.413 [24], clause 9.1.8.5
    byte_array_t s1_mme_name; 
    //s1_setup_response_t s1_mme_name; 

    // 
    x2_global_enb_t x2;
  };
} e2ap_node_comp_id_t;

void free_e2ap_node_comp_id(e2ap_node_comp_id_t* src);

e2ap_node_comp_id_t cp_e2ap_node_comp_id(e2ap_node_comp_id_t const* src);

bool eq_e2ap_node_comp_id(e2ap_node_comp_id_t const* m0, e2ap_node_comp_id_t const* m1);

#endif
