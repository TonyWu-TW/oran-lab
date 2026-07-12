/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef PENDING_EVENT_XAPP_H
#define PENDING_EVENT_XAPP_H

#include "../lib/pending_events.h"
#include "../lib/e2ap/ric_gen_id_wrapper.h"

#include <pthread.h>
#include "../util/alg_ds/ds/assoc_container/bimap.h"

typedef struct{
  pending_event_t ev;
  ric_gen_id_t id;
  int wait_ms;
} pending_event_xapp_t ;

bool eq_pending_event_xapp(pending_event_xapp_t* m0, pending_event_xapp_t* m1);


typedef struct{
  bi_map_t pending; // left: fd, right: pending_event_xapp_t   
  pthread_mutex_t pend_mtx;
} pending_event_xapp_ds_t;


void init_pending_events( pending_event_xapp_ds_t* ds);

void free_pending_events( pending_event_xapp_ds_t* ds);

bool find_pending_event_fd(pending_event_xapp_ds_t* p, int fd);

bool find_pending_event_ev(pending_event_xapp_ds_t* p, pending_event_xapp_t* ev);

void add_pending_event( pending_event_xapp_ds_t* ds, int fd, pending_event_xapp_t* ev );

int* rm_pending_event_ev(pending_event_xapp_ds_t* ds, pending_event_xapp_t* ev );

void rm_pending_event_fd(pending_event_xapp_ds_t* ds, int fd);


#endif

