/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#include "e2ap_plmn.h"

#include <assert.h>
#include <stddef.h>


e2ap_plmn_t cp_e2ap_plmn(const e2ap_plmn_t* src)
{
  assert(src != NULL);

  e2ap_plmn_t dst = {0};

  dst.mcc = src->mcc;
  dst.mnc = src->mnc;
  dst.mnc_digit_len = src->mnc_digit_len;

  return dst;
}

bool eq_e2ap_plmn(const e2ap_plmn_t* m0, const e2ap_plmn_t* m1)
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


int cmp_e2ap_plmn(const e2ap_plmn_t* m0, const e2ap_plmn_t* m1)
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

