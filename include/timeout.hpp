#ifndef INCLUDE_TIMEOUT_HPP_
#define INCLUDE_TIMEOUT_HPP_

#include <stdint.h>

namespace timeout {

// 現在時刻
int64_t now_sec();

// poll timeout(ms) へ
int poll_timeout_ms(int64_t ms, int max_ms);

}  // namespace timeout

#endif  // INCLUDE_TIMEOUT_HPP_