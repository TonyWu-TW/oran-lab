/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef RIC_GEN_ID_H
#define RIC_GEN_ID_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint32_t ric_req_id; // not uint16_t to avoid padding and thus, be able to use memcmp
  uint16_t ric_inst_id;
  uint16_t ran_func_id;
} ric_gen_id_t;

bool eq_ric_gen_id(const ric_gen_id_t* m0, const  ric_gen_id_t* m1);

int cmp_ric_gen_id(const ric_gen_id_t* m0, const  ric_gen_id_t* m1);

ric_gen_id_t copy_ric_gen_id(const ric_gen_id_t* src);

#endif

