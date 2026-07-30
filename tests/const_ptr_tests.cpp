#include <gtest/gtest.h>

import const_ptr;

using AmberRoom::ConstPtr;
using AmberRoom::make_const_ptr;

struct MockStruct{
    int field;

    const int& getField() const{
        return field;
    }
};

TEST(ConstPtrTest, SimpleStructTest) {
    auto ptr = make_const_ptr<MockStruct>(42);
    EXPECT_EQ(ptr->getField(), 42);
}
