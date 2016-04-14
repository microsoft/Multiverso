#include "gtest/gtest.h"


TEST(TestExample, FirstGeSecond) {
    EXPECT_GE(18.0, 17.0);
}

TEST(TestExample, FirstEqualSecond) {
    ASSERT_EQ(0, 0);
}
