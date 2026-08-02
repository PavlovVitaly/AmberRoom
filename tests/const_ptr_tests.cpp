#include <gtest/gtest.h>

import const_ptr;

using AmberRoom::ConstPtr;
using AmberRoom::make_const_ptr;
using AmberRoom::clone_const_ptr;

class MockStruct{
    int field_;

public:
    static inline size_t creating_cnt{};
    static inline size_t copy_creating_cnt{};
    static inline size_t move_creating_cnt{};
    static inline size_t copy_assignment_cnt{};
    static inline size_t move_assignment_cnt{};
    static inline size_t destructed_cnt{};

    static void cleanCnts(){
        creating_cnt = 0;
        copy_creating_cnt = 0;
        move_creating_cnt = 0;
        copy_assignment_cnt = 0;
        move_assignment_cnt = 0;
        destructed_cnt = 0;
    }

    MockStruct(int field): field_(field){
        ++creating_cnt;
    }

    MockStruct(const MockStruct& other): field_(other.field_){
        ++copy_creating_cnt;
    }

    MockStruct(MockStruct&& other): field_(std::move(other.field_)){
        ++move_creating_cnt;
    }

    MockStruct& operator = (const MockStruct& other){
        field_ = other.field_;
        ++copy_assignment_cnt;
        return *this;
    }

    MockStruct& operator = (MockStruct&& other){
        field_ = std::move(other.field_);
        ++copy_assignment_cnt;
        return *this;
    }

    ~MockStruct(){
        ++destructed_cnt;
    }

    const int& getField() const{
        return field_;
    }
};

TEST(ConstPtrTest, SimpleConstructTest) {
    MockStruct::cleanCnts();
    {
        auto ptr = make_const_ptr<MockStruct>(42);
        EXPECT_EQ(ptr->getField(), 42);
    }
    EXPECT_EQ(MockStruct::creating_cnt, 1);
    EXPECT_EQ(MockStruct::copy_creating_cnt, 0);
    EXPECT_EQ(MockStruct::move_creating_cnt, 0);
    EXPECT_EQ(MockStruct::copy_assignment_cnt, 0);
    EXPECT_EQ(MockStruct::move_assignment_cnt, 0);
    EXPECT_EQ(MockStruct::destructed_cnt, 0);
}

TEST(ConstPtrTest, SimpleCopyConstructTest) {
    MockStruct::cleanCnts();
    auto ptr = make_const_ptr<MockStruct>(42);
    auto copyPtr = clone_const_ptr(ptr);
    EXPECT_EQ(copyPtr->getField(), 42);
    EXPECT_EQ(MockStruct::creating_cnt, 1);
    EXPECT_EQ(MockStruct::copy_creating_cnt, 1);
    EXPECT_EQ(MockStruct::move_creating_cnt, 0);
    EXPECT_EQ(MockStruct::copy_assignment_cnt, 0);
    EXPECT_EQ(MockStruct::move_assignment_cnt, 0);
    EXPECT_EQ(MockStruct::destructed_cnt, 0);
}

TEST(ConstPtrTest, SimpleAssignConstructTest) {
    MockStruct::cleanCnts();
    auto ptr = make_const_ptr<MockStruct>(42);
    auto copyPtr = ptr;
    EXPECT_EQ(copyPtr->getField(), 42);
    EXPECT_EQ(MockStruct::creating_cnt, 1);
    EXPECT_EQ(MockStruct::copy_creating_cnt, 0);
    EXPECT_EQ(MockStruct::move_creating_cnt, 0);
    EXPECT_EQ(MockStruct::copy_assignment_cnt, 0);
    EXPECT_EQ(MockStruct::move_assignment_cnt, 0);
    EXPECT_EQ(MockStruct::destructed_cnt, 0);
}


class Base {
public:
    virtual ~Base() = default; // Необходим для dynamic_cast
};

class Derived : public Base {
public:
    void foo() const {}
};

TEST(ConstPtrTest, SimpleUpcastingTest) {
    AmberRoom::ConstPtr<Derived> derivedPtr = AmberRoom::make_const_ptr<Derived>();

    // 1. Auto Upcasting
    AmberRoom::ConstPtr<Base> basePtr = derivedPtr; 

    AmberRoom::ConstPtr<Derived> staticDerived = AmberRoom::static_pointer_cast<Derived>(basePtr);

    if (auto dynamicDerived = AmberRoom::dynamic_pointer_cast<Derived>(basePtr)) {
        dynamicDerived->foo();
    }
}