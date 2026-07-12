
/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E42_SETUP_RESPONSE_H
#define E42_SETUP_RESPONSE_H 

#include "e2_node_connected.h"

typedef struct e42_setup_response {
  uint16_t xapp_id;
  uint8_t len_e2_nodes_conn;
  e2_node_connected_t* nodes;
} e42_setup_response_t;

e42_setup_response_t cp_e42_setup_response(const e42_setup_response_t* src);

bool eq_e42_setup_response(const e42_setup_response_t* m0, const e42_setup_response_t* m1);

#endif

