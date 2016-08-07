#include <evcollect/util/testing.h>

TEST(ConfigParser, Blurb) {
  logf("Blurbed $0", "!");
  ASSERT_EQ(2, 1 + 1);
}
