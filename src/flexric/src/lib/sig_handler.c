/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "sig_handler.h"
#include <errno.h>
#include <stdio.h>
#include <poll.h>
#include <signal.h>

static
void handle_signal(int sig)
{
  printf("\nSignal %d caught.\n", sig);
}

void init_sig_handler(void)
{
  struct sigaction sa;

  sa.sa_handler = handle_signal;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  sigaction(SIGINT,  &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
}

void poll_and_wait_sig(void)
{
  while (1) {
    int ret = poll(NULL, 0, 1000);

    if (ret == -1) {
      if (errno == EINTR) {
        break;
      }

      perror("poll");
      break;
    }
  }
}

