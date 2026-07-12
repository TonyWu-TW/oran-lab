/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */
#include <assert.h>
#include <stdlib.h>
#include "kpm_enc_plain.h"

byte_array_t kpm_enc_event_trigger_plain(kpm_event_trigger_t const* event_trigger)
{
  assert(event_trigger != NULL);
  byte_array_t  ba = {0};
 
  ba.len = sizeof(event_trigger->ms);
  ba.buf = malloc(ba.len);
  assert(ba.buf != NULL && "Memory exhausted");

  memcpy(ba.buf, &event_trigger->ms, ba.len);

  return ba;
}