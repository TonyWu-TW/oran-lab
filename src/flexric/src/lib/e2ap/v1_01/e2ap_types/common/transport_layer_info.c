/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#include "transport_layer_info.h"

bool eq_transport_layer_information(const transport_layer_information_t* m0, const transport_layer_information_t* m1)
{
  if(m0 == m1) return true;

  if(m0 == NULL || m1 == NULL) return false;


  if(eq_byte_array(&m0->address, &m1->address) == false)
    return false;

  if(eq_byte_array(m0->port, m1->port) == false)
    return false;

  return true;
}

