#include <gtest/gtest.h>
#include <gc/gc.h>

import immutable_ptr;

using AmberRoom::ImmutablePtr;
using AmberRoom::make_immutable_ptr;
using AmberRoom::clone_immutable_ptr;

class MockStruct{
    int field_{100};

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

    MockStruct(){
        ++creating_cnt;
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

TEST(ImmutablePtrTest, SimpleConstructTest) { 
    GC_INIT();
    MockStruct::cleanCnts();
    {
        auto ptr = make_immutable_ptr<MockStruct>(12345);
        EXPECT_EQ(ptr->getField(), 12345);
    }
    
    
    for (int i = 0; i < 50; ++i) {
        GC_clear_roots(); 
        GC_gcollect();
        GC_invoke_finalizers();
    }
    
    EXPECT_EQ(MockStruct::creating_cnt, 1);
    EXPECT_EQ(MockStruct::copy_creating_cnt, 0);
    EXPECT_EQ(MockStruct::move_creating_cnt, 0);
    EXPECT_EQ(MockStruct::copy_assignment_cnt, 0);
    EXPECT_EQ(MockStruct::move_assignment_cnt, 0);
    EXPECT_EQ(MockStruct::destructed_cnt, 0);
}

TEST(ImmutablePtrTest, SimpleCopyConstructTest) {
    MockStruct::cleanCnts();
    auto ptr = make_immutable_ptr<MockStruct>(42);
    auto copyPtr = clone_immutable_ptr(ptr);
    EXPECT_EQ(copyPtr->getField(), 42);
    EXPECT_EQ(MockStruct::creating_cnt, 1);
    EXPECT_EQ(MockStruct::copy_creating_cnt, 1);
    EXPECT_EQ(MockStruct::move_creating_cnt, 0);
    EXPECT_EQ(MockStruct::copy_assignment_cnt, 0);
    EXPECT_EQ(MockStruct::move_assignment_cnt, 0);
    EXPECT_EQ(MockStruct::destructed_cnt, 0);
}

TEST(ImmutablePtrTest, SimpleAssignConstructTest) {
    MockStruct::cleanCnts();
    auto ptr = make_immutable_ptr<MockStruct>(42);
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

TEST(ImmutablePtrTest, SimpleUpcastingTest) {
    AmberRoom::ImmutablePtr<Derived> derivedPtr = AmberRoom::make_immutable_ptr<Derived>();

    // Auto Upcasting
    AmberRoom::ImmutablePtr<Base> basePtr = derivedPtr; 

    AmberRoom::ImmutablePtr<Derived> staticDerived = AmberRoom::static_pointer_cast<Derived>(basePtr);

    if (auto dynamicDerived = AmberRoom::dynamic_pointer_cast<Derived>(basePtr)) {
        dynamicDerived->foo();
    }
}
