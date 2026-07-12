/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "e2_node_ric_id.h"

#include <assert.h>
#include <stdlib.h>

e2_node_ric_id_t cp_e2_node_ric_id(e2_node_ric_id_t const* src)
{
  assert(src != NULL);

  e2_node_ric_id_t dst = {0};
  dst.ric_id = src->ric_id;
  dst.ric_req_type = src->ric_req_type;
  dst.e2_node_id = cp_global_e2_node_id(&src->e2_node_id);

  return dst;
}


void free_e2_node_ric_id(e2_node_ric_id_t* src)
{
  assert(src != NULL);

  free_global_e2_node_id(&src->e2_node_id);
}


void free_e2_node_ric_id_wrapper(void* src)
{
  assert(src != NULL);
  return free_e2_node_ric_id(src);
}

