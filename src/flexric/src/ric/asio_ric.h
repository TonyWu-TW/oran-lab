/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef ASYNC_INPUT_OUTPUT_RIC_H
#define ASYNC_INPUT_OUTPUT_RIC_H

#include <stddef.h>

typedef struct{
  // epoll based fd
  int efd; 

} asio_ric_t;


void init_asio_ric(asio_ric_t* io);

void add_fd_asio_ric(asio_ric_t* io, int fd);

int create_timer_ms_asio_ric(asio_ric_t* io, long initial_ms, long interval_ms);

void rm_fd_asio_ric(asio_ric_t* io, int fd);

typedef struct{
  int fd[64];
  int len;
} fd_read_t;

fd_read_t event_asio_ric(asio_ric_t const* io);

#endif

