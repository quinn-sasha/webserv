#ifndef INCLUDE_TIMEOUTMANAGER_HPP_
#define INCLUDE_TIMEOUTMANAGER_HPP_

#include <stdint.h>
#include <map>
#include <vector>

class MonitoredFdHandler;

class TimeoutManager {
 public:
  TimeoutManager();
  ~TimeoutManager();

  void add_timeout(int fd, MonitoredFdHandler* handler);
  void remove_timeout(int fd);
  void update_timeout(int fd);

  int get_next_timeout_ms() const;
  std::vector<int> get_timedout_fds();

 private:
  std::map<int, MonitoredFdHandler*> fd_to_timeout_;
  std::map<int, int64_t> fd_to_last_deadline_;
  std::multimap<int64_t, int> deadlines_;

  void erase_from_deadlines_(int fd);
  int64_t now_() const;
};



#endif  // INCLUDE_TIMEOUTMANAGER_HPP_
