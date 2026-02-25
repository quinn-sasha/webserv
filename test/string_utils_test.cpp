#include "string_utils.hpp"

#include <gtest/gtest.h>

#include <list>
#include <string>

TEST(StringUtilsTest, SplitString) {
  std::string target = "gzip, deflate, chunked";
  std::list<std::string> result = split_string(target, ", ");
  std::list<std::string> expected = {"gzip", "deflate", "chunked"};
  EXPECT_EQ(result, expected);
}
