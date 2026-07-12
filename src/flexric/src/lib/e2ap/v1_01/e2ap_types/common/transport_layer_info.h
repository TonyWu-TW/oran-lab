/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef E2AP_TRANSPORT_LAYER_INFO_H
#define E2AP_TRANSPORT_LAYER_INFO_H

#include <stdint.h>
#include "util/byte_array.h"

typedef struct transport_layer_information {
  byte_array_t address;
  byte_array_t* port; // optional
} transport_layer_information_t;

bool eq_transport_layer_information(const transport_layer_information_t* m0, const transport_layer_information_t* m1);

#endif
