/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef UTIL_COMPARE_H
#define UTIL_COMPARE_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

// Equality file descriptors
static inline
bool eq_fd(const void* key1, const void* key2 )
{
  assert(key1 != NULL);
  assert(key2 != NULL);

  int* fd1 = (int*)key1;
  int* fd2 = (int*)key2;

  return *fd1 == *fd2;
}

// Comparation file descriptors
static inline 
int cmp_fd(void const* fd_v1, void const* fd_v2)
{
  assert(fd_v1 != NULL);
  assert(fd_v2 != NULL);
  int* fd1 = (int*)fd_v1;
  int* fd2 = (int*)fd_v2;

  if(*fd1 < *fd2) return 1;
  if(*fd1 == *fd2) return 0;
  return -1;
}




//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


static inline
bool eq_ran_func_id(const void* value, const void* key)
{
  uint16_t* ran_func_id1 = (uint16_t*)value;
  uint16_t* ran_func_id2 = (uint16_t*)key;
  assert(ran_func_id1 != NULL);
  assert(ran_func_id2 != NULL);

  return *ran_func_id1 == *ran_func_id2;
}

// Comparing RAN Function IDs
static inline
int cmp_ran_func_id(const void* a_v, const void* b_v)
{
  assert(a_v != NULL);
  assert(b_v != NULL);

  uint16_t* a = (uint16_t*)a_v;
  uint16_t* b = (uint16_t*)b_v;

  if(*a < *b) return 1;
  if(*a == *b) return 0;
  return -1; 
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////



#endif

