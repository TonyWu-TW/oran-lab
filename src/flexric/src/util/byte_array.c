/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#include "byte_array.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

byte_array_t copy_byte_array(byte_array_t src)
{
  byte_array_t dst = {0};
  dst.buf = malloc(src.len);
  memcpy(dst.buf, src.buf, src.len);
  dst.len = src.len;
  return dst;
}

void free_byte_array(byte_array_t ba)
{
  free(ba.buf);
}

bool eq_byte_array(const byte_array_t* m0, const byte_array_t* m1)
{
  if(m0 == m1)
    return true;

  if(m0 == NULL || m1 == NULL)
    return false;

  if(m0->len != m1->len)
    return false;

  const int rc = memcmp(m0->buf, m1->buf, m0->len); 
  if(rc != 0)
    return false;

  return true;
}

byte_array_t cp_str_to_ba(const char* str)
{
  assert(str != NULL);
  
  const size_t sz = strlen(str);

  byte_array_t dst = {.len = sz};

  dst.buf = calloc(sz,sizeof(uint8_t));
  assert(dst.buf != NULL && "Memory exhausted");

  memcpy(dst.buf, str, sz);

  return dst;
}

char* cp_ba_to_str(const byte_array_t ba)
{
  assert(ba.len > 0);

  const size_t sz = ba.len;
  char* str = calloc(sz+1, sizeof(char));
  assert(str != NULL && "Memory exhausted");

  memcpy(str, ba.buf, sz);
  str[sz] = '\0';

  return str;
}

int cmp_str_ba(char const* str, byte_array_t ba)
{
  assert(str != NULL);
  assert(ba.len > 0);

  size_t pos = 0;
  while(str[pos] != '\0' && pos < ba.len){
    if(str[pos] < ba.buf[pos])
      return -1;
    else if(str[pos] > ba.buf[pos])
      return 1;
    pos++;
  }

  if(pos != ba.len)
    return -1;
  else if(str[pos] != '\0')
    return 1;

  return 0;
}
