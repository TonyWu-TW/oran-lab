/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E2AP_PLMN_IDENTITY_H
#define E2AP_PLMN_IDENTITY_H

#ifdef __cplusplus
extern "C" {
#endif


#include <stdbool.h>
#include <stdint.h>



typedef struct {
  uint16_t mcc;
  uint16_t mnc;
  uint8_t mnc_digit_len;
} e2ap_plmn_t;

bool eq_e2ap_plmn(const e2ap_plmn_t* m0, const e2ap_plmn_t* m1);

e2ap_plmn_t cp_e2ap_plmn(const e2ap_plmn_t* src);

int cmp_e2ap_plmn(const e2ap_plmn_t* m0, const e2ap_plmn_t* m1);

#ifdef __cplusplus
}
#endif


#endif

