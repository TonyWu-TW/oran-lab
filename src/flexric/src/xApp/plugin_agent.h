/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef E2_PLUGIN_AGENT_H
#define E2_PLUGIN_AGENT_H

#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#include "util/alg_ds/ds/assoc_container/assoc_generic.h"

#include "sm/sm_agent.h"
#include "sm/sm_io.h"


typedef struct
{
  sm_io_ag_ran_t io;
  const char* dir_path;

  // Registered SMs
  assoc_rb_tree_t sm_ds; // key: ran_func_id, value: sm_agent_t* 
  pthread_mutex_t sm_ds_mtx;

//  int sockfd;
  pthread_t thread_rx;

  atomic_bool flag_shutdown;
} plugin_ag_t;


void init_plugin_ag(plugin_ag_t* p, const char* path, sm_io_ag_ran_t io);

void free_plugin_ag(plugin_ag_t* p);

void load_plugin_ag(plugin_ag_t* p, const char* path);

void unload_plugin_ag(plugin_ag_t* p, uint16_t key);

sm_agent_t* sm_plugin_ag(plugin_ag_t* p, uint16_t key);

size_t size_plugin_ag(plugin_ag_t* p);


#endif

