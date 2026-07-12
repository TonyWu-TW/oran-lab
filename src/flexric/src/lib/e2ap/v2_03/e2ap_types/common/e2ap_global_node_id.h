/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E2AP_GLOBAL_NODE_ID_H
#define E2AP_GLOBAL_NODE_ID_H

#ifdef __cplusplus
extern "C" {
#endif

#include "e2ap_plmn.h"
#include "../../../../../util/ngran_types.h"
#include "../../../../3gpp/ie/e2ap_gnb_id.h"

#include <stdbool.h>
#include <stdint.h>


typedef struct global_e2_node_id {
  ngran_node_t type;
  e2ap_plmn_t plmn;
  e2ap_gnb_id_t nb_id;
  uint64_t *cu_du_id;
} global_e2_node_id_t;

global_e2_node_id_t cp_global_e2_node_id(global_e2_node_id_t const* src);

void free_global_e2_node_id(global_e2_node_id_t* src);
 
void free_global_e2_node_id_wrapper(void* src);

bool eq_global_e2_node_id(const global_e2_node_id_t* m0, const global_e2_node_id_t* m1); 

bool eq_global_e2_node_id(const global_e2_node_id_t* m0, const global_e2_node_id_t* m1);
 
bool eq_global_e2_node_id_wrapper(const void* m0_v, const void* m1_v );
 
int cmp_global_e2_node_id(const global_e2_node_id_t* m0, const global_e2_node_id_t* m1);
 
int cmp_global_e2_node_id_wrapper(const void* m0_v, const void* m1_v);

#ifdef __cplusplus
}
#endif

#endif

