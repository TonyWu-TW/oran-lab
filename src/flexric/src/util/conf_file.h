/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef FLEXRIC_CONFIGURATION_FILE_H
#define FLEXRIC_CONFIGURATION_FILE_H 

#include <stdint.h>
#define FR_IP_ADDRESS_LEN 16
#define FR_CONF_FILE_LEN 128

typedef struct {
  // Option 1: overwrite the default values at run time
  char ip[FR_IP_ADDRESS_LEN];
  char db_dir[FR_CONF_FILE_LEN];
  char db_name[FR_CONF_FILE_LEN];

  // Option 2: read from file
  char conf_file[FR_CONF_FILE_LEN];
  char libs_dir[FR_CONF_FILE_LEN];
} fr_args_t;

fr_args_t init_fr_args(int argc, char* argv[]);

char* get_near_ric_ip(fr_args_t const*);

char* get_conf_db_dir(fr_args_t const*);

char* get_conf_db_name(fr_args_t const*);

#endif

