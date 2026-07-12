/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E42_SETUP_REQUEST_H
#define E42_SETUP_REQUEST_H 

#include "common/e2ap_ran_function.h"

typedef struct e42_setup_request {
  ran_function_t* ran_func_item;
  size_t len_rf;
} e42_setup_request_t;


e42_setup_request_t cp_e42_setup_request(const e42_setup_request_t* src);

void free_e42_setup_request(e42_setup_request_t* src);

bool eq_e42_setup_request(const e42_setup_request_t* m0, const e42_setup_request_t* m1);

#endif

