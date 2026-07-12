/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#include "e2ap_global_ric_id.h"
#include <stddef.h>


bool eq_global_ric_id(const global_ric_id_t* m0, const global_ric_id_t* m1)
{
  if(m0 == m1) return true;

  if(m0 == NULL || m1 == NULL) return false;

  if(eq_e2ap_plmn(&m0->plmn, &m1->plmn) == false)
    return false;

  if(m0->near_ric_id.double_word != m1->near_ric_id.double_word) 
    return false;

  return true;
};
