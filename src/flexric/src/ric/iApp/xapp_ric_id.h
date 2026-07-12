/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef XAPP_RIC_ID_H
#define XAPP_RIC_ID_H 

#include "../../lib/e2ap/ric_gen_id_wrapper.h"
#include <stdbool.h>
#include <stdint.h>


typedef struct{
  ric_gen_id_t ric_id;
  uint16_t xapp_id;
} xapp_ric_id_t;

int cmp_xapp_ric_gen_id(xapp_ric_id_t const* m0,  xapp_ric_id_t const* m1);
int cmp_xapp_ric_gen_id_wrapper(void const* m0, void const* m1);

bool eq_xapp_ric_gen_id(xapp_ric_id_t const* m0, xapp_ric_id_t const* m1);
bool eq_xapp_ric_gen_id_wrapper(void const* m0, void const* m1);
bool eq_xapp_id(uint16_t m0, uint16_t m1);

bool eq_xapp_id_gen_wrapper(void const* m0, void const* m1);
bool eq_xapp_ric_gen_id_wrapper(void const* m0, void const* m1);

typedef struct{
  bool has_value;
  union{
    char* error;
    xapp_ric_id_t xapp_ric_id;
  };
} xapp_ric_id_xpct_t;

#endif

