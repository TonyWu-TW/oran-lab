/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef SQLITE3_WRAPPER_XAPP_H
#define SQLITE3_WRAPPER_XAPP_H 

#include "../../../sm/agent_if/read/sm_ag_if_rd.h"
#include "../../../lib/e2ap/e2ap_global_node_id_wrapper.h" 

#include "sqlite3.h"

void init_db_sqlite3(sqlite3** db, char const* db_filename);

void close_db_sqlite3(sqlite3* db);

void write_db_sqlite3(sqlite3* db, global_e2_node_id_t const* id, sm_ag_if_rd_t const* rd);

#endif

