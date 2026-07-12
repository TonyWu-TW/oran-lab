/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#include "ric_gen_id.h"
#include <assert.h>
#include <stdlib.h>

bool eq_ric_gen_id(const ric_gen_id_t* m0, const  ric_gen_id_t* m1)
{
  return m0->ric_req_id == m1->ric_req_id 
    && m0->ric_inst_id == m1->ric_inst_id
    && m0->ran_func_id == m1->ran_func_id;
}

int cmp_ric_gen_id(const ric_gen_id_t* m0, const  ric_gen_id_t* m1)
{
  assert(m0 != NULL);
  assert(m1 != NULL);

  if(m0->ran_func_id < m1->ran_func_id) return 1;
  if(m0->ran_func_id > m1->ran_func_id) return -1;

  if(m0->ric_inst_id < m1->ric_inst_id) return 1;
  if(m0->ric_inst_id > m1->ric_inst_id) return -1;

  if(m0->ric_req_id < m1->ric_req_id) return 1;
  if(m0->ric_req_id > m1->ric_req_id) return -1;

  return 0;
}

ric_gen_id_t copy_ric_gen_id(const ric_gen_id_t* src)
{
  ric_gen_id_t dst = {
    .ric_req_id = src->ric_req_id,
    .ric_inst_id = src->ric_inst_id,
    .ran_func_id = src->ran_func_id
  };

  return dst;
}
