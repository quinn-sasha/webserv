#include <signal.h>
#include <stdio.h>

#include <csignal>

#include "config_utils.hpp"

extern volatile sig_atomic_t g_running;

void turn_off_running_status(int) { g_running = false; }

void set_signal_handler(int signum, void (*handler)(int)) {
  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = handler;
  if (sigaction(signum, &sa, NULL) == -1) {
    error_exit("sigaction()");
  }
}
