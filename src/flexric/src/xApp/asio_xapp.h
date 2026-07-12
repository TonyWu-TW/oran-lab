/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef ASYNC_INPUT_OUTPUT_XAPP_H
#define ASYNC_INPUT_OUTPUT_XAPP_H

typedef struct{
  // epoll based fd
  int efd; 

} asio_xapp_t;


void init_asio_xapp(asio_xapp_t* io);

void add_fd_asio_xapp(asio_xapp_t* io, int fd);

int create_timer_ms_asio_xapp(asio_xapp_t* io, long initial_ms, long interval_ms);

void rm_fd_asio_xapp(asio_xapp_t* io, int fd);

int event_asio_xapp(asio_xapp_t const* io);


#endif

