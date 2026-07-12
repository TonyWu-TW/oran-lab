/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef E2_NODE_ARR_H
#define E2_NODE_ARR_H 

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../e2ap/e2_node_connected_wrapper.h"

typedef struct{
  uint8_t len;
  e2_node_connected_t* n;
} e2_node_arr_t;

void free_e2_node_arr(e2_node_arr_t*);

#ifdef __cplusplus
}
#endif

#endif

