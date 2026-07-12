/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#include "plmn_identity.h"

#include <assert.h>
#include <stddef.h>

void free_e2sm_plmn(e2sm_plmn_t* src) // 6.2.3.1
{
  assert(src != NULL);

 // uint16_t mcc;
 // uint16_t mnc;
 // uint8_t mnc_digit_len;

}


 e2sm_plmn_t cp_e2sm_plmn(const e2sm_plmn_t* src)
{
  assert(src != NULL);

  e2sm_plmn_t dst = {0};

  dst.mcc = src->mcc;
  dst.mnc = src->mnc;
  dst.mnc_digit_len = src->mnc_digit_len;

  return dst;
}

bool eq_e2sm_plmn(const e2sm_plmn_t* m0, const e2sm_plmn_t* m1)
{
  if(m0 == m1) return true;

  if(m0 == NULL || m1 == NULL) return false;

  if(m0->mcc != m1->mcc) 
    return false;

  if(m0->mnc != m1->mnc)
    return false;

  if(m0->mnc_digit_len != m1->mnc_digit_len)
    return false;

  return true;
}


int cmp_e2sm_plmn(const e2sm_plmn_t* m0, const e2sm_plmn_t* m1)
{
  assert(m0 != NULL);
  assert(m1 != NULL);

  if(m0->mcc < m1->mcc)
    return -1;
  else if(m0->mcc > m1->mcc )
    return 1;

  if(m0->mnc < m1->mnc)
    return -1;
  else if(m0->mnc > m1->mnc )
    return 1;

  if(m0->mnc_digit_len < m1->mnc_digit_len)
    return -1;
  else if(m0->mnc_digit_len  > m1->mnc_digit_len)
    return 1;

  return 0;
}

