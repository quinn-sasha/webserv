#include "TimeoutManager.hpp"
#include "MonitoredFdHandler.hpp"
#include "Server.hpp"
#include "SystemError.hpp"
#include <ctime>
#include <algorithm>
#include <poll.h>
#include <vector>
#include <iostream>

TimeoutManager::TimeoutManager() {}
TimeoutManager::~TimeoutManager() {}

int64_t TimeoutManager::now_() const {
  return static_cast<int64_t>(std::time(NULL));
}

int TimeoutManager::get_next_timeout_ms() const {
  if (deadlines_.empty()) 
    return 1000;
  
  int64_t diff = deadlines_.begin()->first - now_();
  
  if (diff <= 0) 
    return 0;
  return static_cast<int>(diff * 1000);
}

std::vector<int> TimeoutManager::get_timedout_fds() {
  std::vector<int> timedout;
  int64_t now = now_();
  
  while (!deadlines_.empty() && deadlines_.begin()->first <= now) {
    int fd = deadlines_.begin()->second;
    timedout.push_back(fd);
    deadlines_.erase(deadlines_.begin());
    fd_to_last_deadline_.erase(fd);
  }
  return timedout;
}

void TimeoutManager::erase_from_deadlines_(int fd) {
  std::map<int, int64_t>::iterator it = fd_to_last_deadline_.find(fd);
  if (it == fd_to_last_deadline_.end()) return;

  int64_t deadline = it->second;
  std::pair<std::multimap<int64_t, int>::iterator, std::multimap<int64_t, int>::iterator> range = 
      deadlines_.equal_range(deadline);
  for (std::multimap<int64_t, int>::iterator mit = range.first; mit != range.second; ++mit) {
    if (mit->second == fd) {
      deadlines_.erase(mit);
      break;
    }
  }
  fd_to_last_deadline_.erase(it);
}

void TimeoutManager::add_timeout(int fd, MonitoredFdHandler* handler) {
  if (handler == NULL) 
    return;
  fd_to_timeout_[fd] = handler;
  update_timeout(fd);
}

void TimeoutManager::remove_timeout(int fd) {
  fd_to_timeout_.erase(fd);
  erase_from_deadlines_(fd);
}

void TimeoutManager::update_timeout(int fd) {
  std::map<int, MonitoredFdHandler*>::iterator hit = fd_to_timeout_.find(fd);
  if (hit == fd_to_timeout_.end()) 
    return;
  
  MonitoredFdHandler* handler = hit->second;
  if (!handler->has_deadline()) {
    erase_from_deadlines_(fd);
    fd_to_timeout_[fd] = handler;
    return;
  }

  int64_t new_deadline = handler->deadline_sec();
  
  std::map<int, int64_t>::iterator lit = fd_to_last_deadline_.find(fd);
  if (lit != fd_to_last_deadline_.end()) {
    if (lit->second == new_deadline) 
      return;
    erase_from_deadlines_(fd);
  }

  deadlines_.insert(std::make_pair(new_deadline, fd));
  fd_to_last_deadline_[fd] = new_deadline;
}

