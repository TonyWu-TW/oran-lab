/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef E2_PLUGIN_RIC_H
#define E2_PLUGIN_RIC_H 

#include <stddef.h>
#include <stdint.h>

#include "util/alg_ds/ds/assoc_container/assoc_generic.h"

#include "sm/sm_ric.h"
#include "sm/sm_io.h"

typedef struct
{
  const char* dir_path;

  // Registered SMs
  assoc_rb_tree_t sm_ds; // key: ran_func_id, value: sm_ric_t* 

} plugin_ric_t;

void init_plugin_ric(plugin_ric_t* p, const char* dir_path);

void free_plugin_ric(plugin_ric_t* p);

void load_plugin_ric(plugin_ric_t* p, const char* file_path);

void unload_plugin_ric(plugin_ric_t* p, uint16_t key);

sm_ric_t* sm_plugin_ric(plugin_ric_t* p, uint16_t key);

size_t size_plugin_ric(plugin_ric_t* p);

void tx_plugin_ric(plugin_ric_t* p, size_t len, char const file_path[len]);

#endif

