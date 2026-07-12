/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */




#include "e42_setup_request.h"

#include <assert.h>
#include <stdlib.h>

e42_setup_request_t cp_e42_setup_request(const e42_setup_request_t* src)
{
  assert(src != NULL);

 e42_setup_request_t dst = {0};

 if(src->len_rf > 0){
  dst.ran_func_item = calloc(src->len_rf, sizeof(ran_function_t) );
  assert(dst.ran_func_item != NULL && "Memory exausted");
 }

 for(size_t i = 0; i < src->len_rf; ++i){
  dst.ran_func_item[i] = cp_ran_function(&src->ran_func_item[i]);
 }

 dst.len_rf = src->len_rf;

 return dst;
}

void free_e42_setup_request(e42_setup_request_t* src)
{
  assert(src != NULL);
  for(size_t i = 0; i < src->len_rf; ++i){
    free_ran_function(&src->ran_func_item[i]);
  }
  free(src->ran_func_item);
}

bool eq_e42_setup_request(const e42_setup_request_t* m0, const e42_setup_request_t* m1)
{
  if(m0 == m1)
    return true;

  if(m0 == NULL && m1 == NULL)
    return true;

  if(m0 != NULL && m1 == NULL)
    return false;

  if(m0 == NULL && m1 != NULL)
    return false;

  if(m0->len_rf != m1->len_rf)
    return false;

  for(size_t i = 0; i < m0->len_rf; ++i){
    if(eq_ran_function(&m0->ran_func_item[i], &m1->ran_func_item[i]) == false)
      return false;
  }
  return true;
}

