/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef ASYNC_INPUT_OUTPUT_IAPP_H
#define ASYNC_INPUT_OUTPUT_IAPP_H


typedef struct{

  // epoll based fd
  int efd; 

} asio_iapp_t;

typedef enum
{
  NET_PKT_ASIO_EVENT,
  IND_MSG_ASIO_EVENT,
  PENDING_TIMEOUT_EVENT,

} asio_ev_t ;


void init_asio_iapp(asio_iapp_t* io);

void add_fd_asio_iapp(asio_iapp_t* io, int fd);

void rm_fd_asio_iapp(asio_iapp_t* io, int fd);

int create_timer_ms_asio_iapp(asio_iapp_t* io, long initial_ms, long interval_ms);

int event_asio_iapp(asio_iapp_t const* io);

#endif

