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

TEST(StringUtilsTest, ConvertToInteger) {
  int num;
  int res = convert_to_integer(num, "2147483647", 10);
  EXPECT_EQ(res, 0);
  EXPECT_EQ(num, 2147483647);

  res = convert_to_integer(num, "-2147483648", 10);
  EXPECT_EQ(res, 0);
  EXPECT_EQ(num, -2147483648);

  res = convert_to_integer(num, "0", 10);
  EXPECT_EQ(res, 0);
  EXPECT_EQ(num, 0);

  res = convert_to_integer(num, "98ab", 10);
  EXPECT_EQ(res, -1);

  res = convert_to_integer(num, "92233720368547758070000000", 10);
  EXPECT_EQ(res, -1);

  res = convert_to_integer(num, "af", 16);
  EXPECT_EQ(res, 0);
  EXPECT_EQ(num, 175);
}
