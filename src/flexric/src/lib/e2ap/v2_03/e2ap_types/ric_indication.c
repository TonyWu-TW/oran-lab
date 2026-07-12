/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#include "ric_indication.h"

#include <assert.h>
#include <string.h>

static
bool eq_sn(uint16_t* m0, uint16_t* m1)
{
  if(m0 == m1) return true;

  if(m0 == NULL || m1 == NULL)
    return false;

  if(*m0 != *m1) 
    return false;

  return true;
} 

bool eq_ric_indication(const ric_indication_t* m0, const ric_indication_t* m1)
{
  if(m0 == m1) return true;

  if(m0 == NULL || m1 == NULL) 
    return false;

  if(eq_ric_gen_id(&m0->ric_id, &m1->ric_id) == false)
    return false;

  if(m0->action_id != m1->action_id)
    return false;

  if(eq_sn(m0->sn, m1->sn) == false )
    return false;

  if(m0->type != m1->type) 
    return false;

  if(eq_byte_array(&m0->hdr,&m1->hdr) == false)
    return false;

   if(eq_byte_array(&m0->msg,&m1->msg) == false)
    return false;

   if(eq_byte_array(m0->call_process_id, m1->call_process_id) == false)
    return false;

  return true;
}

ric_indication_t mv_ric_indication(ric_indication_t* src)
{
  assert(src != NULL);

  ric_indication_t dst = {0};

  dst.ric_id = src->ric_id;
  dst.action_id = src->action_id;
  dst.sn = src->sn; // optional
  dst.type = src->type;
  dst.hdr = src->hdr;
  dst.msg = src->msg;
  dst.call_process_id = src->call_process_id; // optional

  memset(src, 0, sizeof(ric_indication_t) );

  return dst;
}

