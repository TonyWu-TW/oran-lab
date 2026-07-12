/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */
 

#ifndef E2_NODE_ARR_XAPP_H
#define E2_NODE_ARR_XAPP_H 

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "e2_node_connected_xapp.h"

typedef struct{
  uint8_t len;
  e2_node_connected_xapp_t* n;
} e2_node_arr_xapp_t;

void free_e2_node_arr_xapp(e2_node_arr_xapp_t*);

#ifdef __cplusplus
}
#endif

#endif

