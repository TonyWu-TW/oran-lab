/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "e2_node_arr.h"

#include <assert.h>
#include <stdlib.h>

void free_e2_node_arr(e2_node_arr_t* xapp)
{
  assert(xapp != NULL);

  for(size_t i = 0; i < xapp->len; ++i){
    e2_node_connected_t* n = &xapp->n[i]; 
    free_e2_node_connected(n);
  }
 
  if(xapp->len > 0){
    free(xapp->n);
  }
}

