/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "e42_ric_control_request.h"

bool eq_e42_ric_control_request(const e42_ric_control_request_t* m0, const  e42_ric_control_request_t* m1)
{
  if(m0 == NULL && m1 == NULL)
    return true;

  if(m0 == NULL)
    return false;

  if(m1 == NULL)
    return false;

  if(m0->xapp_id != m1->xapp_id)
    return false;

  bool rv = eq_global_e2_node_id(&m0->id, &m1->id);
  if(rv == false)
    return false;

  rv = eq_ric_control_request(&m0->ctrl_req, &m1->ctrl_req);

  return rv;
}

