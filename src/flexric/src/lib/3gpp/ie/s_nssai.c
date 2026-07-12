/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "s_nssai.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

bool eq_s_nssai_e2sm(const s_nssai_e2sm_t* m0, const s_nssai_e2sm_t* m1)
{
  if(m0 == m1) return true;

  if(m0 == NULL || m1 == NULL) return false;

  if(m0->sST != m1->sST)
    return false;

  if(m0->sD != NULL || m1->sD != NULL){
    if(m0->sD == NULL || m1->sD == NULL)
      return false;
    if(*m0->sD != *m1->sD)
      return false;
  }


  return true;
}

void free_s_nssai_e2sm( s_nssai_e2sm_t* src)
{
  assert(src != NULL);

  if(src->sD != NULL)
    free(src->sD);

}
