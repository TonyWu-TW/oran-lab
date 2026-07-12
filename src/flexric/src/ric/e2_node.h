/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E2_NODE_RIC_STRUCT_H
#define E2_NODE_RIC_STRUCT_H 

#include <stddef.h>

#include "../lib/e2ap/e2ap_global_node_id_wrapper.h"
#include "../lib/e2ap/e2_setup_response_wrapper.h"


typedef struct{
  global_e2_node_id_t id;

  accepted_ran_function_t* accepted;
  size_t len_acc;

} e2_node_t ;


void init_e2_node(e2_node_t* n, global_e2_node_id_t const* id, size_t len_acc, accepted_ran_function_t accepted[len_acc]);

void free_e2_node(e2_node_t* n);

void free_e2_node_void(void *n);

e2_node_t cp_e2_node(e2_node_t const* n);

#endif

