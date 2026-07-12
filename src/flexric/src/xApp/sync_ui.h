/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef MIR_SYNCRONIZE_USER_INTERFACE_H
#define MIR_SYNCRONIZE_USER_INTERFACE_H 

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct{
  pthread_cond_t cv_sync; // = PTHREAD_COND_INITIALIZER;
  pthread_mutex_t mtx_sync; // = PTHREAD_MUTEX_INITIALIZER;
  int wait_ms;
  bool flag_sync; // = false;
  bool msg_ack; // = false;
} sync_ui_t;

void init_sync_ui(sync_ui_t* s);

void free_sync_ui(sync_ui_t* s);

void cond_wait_sync_ui(sync_ui_t* s, uint32_t ms);

void signal_sync_ui(sync_ui_t* s); 

#endif

