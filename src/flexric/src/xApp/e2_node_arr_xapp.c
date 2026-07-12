/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "e2_node_arr_xapp.h"

#include <assert.h>
#include <stdlib.h>

void free_e2_node_arr_xapp(e2_node_arr_xapp_t* src)
{
  assert(src != NULL);

  for(size_t i = 0; i < src->len; ++i){
   free_e2_node_connected_xapp(&src->n[i]);
  }

  if(src->n != NULL)
    free(src->n);
}

