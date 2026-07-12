/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef BYTE_ARRAY_H
#define BYTE_ARRAY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct {
  size_t len;
  uint8_t *buf;
} byte_array_t;

byte_array_t copy_byte_array(byte_array_t src);

void free_byte_array(byte_array_t ba);

bool eq_byte_array(const byte_array_t* m0, const byte_array_t* m1);

byte_array_t cp_str_to_ba(const char* str);
char* cp_ba_to_str(const byte_array_t ba);
int cmp_str_ba(char const* str, byte_array_t ba);

#endif
