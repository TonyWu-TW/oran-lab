/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "e2ap_gnb_id.h"

bool eq_e2ap_gnb_id(e2ap_gnb_id_t m0,  e2ap_gnb_id_t m1)
{
  if(m0.nb_id != m1.nb_id)
    return false;

  if(m0.unused != m1.unused)
    return false;

  return true;
}



