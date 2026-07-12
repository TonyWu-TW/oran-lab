/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef ASYNC_INPUT_OUTPUT_AGENT_H
#define ASYNC_INPUT_OUTPUT_AGENT_H

typedef struct{
  int r;
  int w;
} fd_pair_t;

typedef struct{
  // epoll based fd
  int efd; 
  // Aperiodic events
  // pipe fd, for communication with epoll
  fd_pair_t pipe; 
} asio_agent_t;

void init_asio_agent(asio_agent_t* io);

void add_fd_asio_agent(asio_agent_t* io, int fd);

void rm_fd_asio_agent(asio_agent_t* io, int fd);

int create_timer_ms_asio_agent(asio_agent_t* io, long initial_ms, long interval_ms);

int event_asio_agent(asio_agent_t const* io);

#endif

