/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#include "e2ap_cause.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

bool eq_cause(const cause_t* m0, const cause_t* m1)
{
  if(m0 == m1) return true;

  if(m0 == NULL || m1 == NULL)
    return false;

  const int rc = memcmp(m0, m1, sizeof(cause_t));
  if( rc != 0) return false;

  return true;
}

cause_t cp_cause(cause_t const* src)
{
   assert(src != NULL);
   cause_t dst = *src;
   return dst;
}

