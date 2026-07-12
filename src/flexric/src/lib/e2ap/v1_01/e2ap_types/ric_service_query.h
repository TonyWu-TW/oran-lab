/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef RIC_SERVICE_QUERY_H
#define RIC_SERVICE_QUERY_H

#include <stddef.h>
#include "common/e2ap_ran_function_id_rev.h"

typedef struct {
  e2ap_ran_function_id_rev_t* accepted;
  size_t len_accepted;
} ric_service_query_t;

bool eq_ric_service_query(const ric_service_query_t* m0, const ric_service_query_t* m1);

#endif

