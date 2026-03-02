#ifndef INCLUDE_SIGNAL_UTILS_HPP_
#define INCLUDE_SIGNAL_UTILS_HPP_

void turn_off_running_status(int signum);
void set_signal_handler(int signum, void (*handler)(int));

#endif  // INCLUDE_SIGNAL_UTILS_HPP_
