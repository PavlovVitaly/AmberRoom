#include <gtest/gtest.h>
#include <gc/gc.h>
#include <stdexcept>

import immutable_ptr;

using AmberRoom::ImmutablePtr;
using AmberRoom::make_flat_immutable_ptr;
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
        ++move_assignment_cnt;
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
    MockStruct::cleanCnts();
    {
        auto ptr = make_flat_immutable_ptr<MockStruct>(12345);
        EXPECT_EQ(ptr->getField(), 12345);
    }
    EXPECT_EQ(MockStruct::creating_cnt, 1);
    EXPECT_EQ(MockStruct::copy_creating_cnt, 0);
    EXPECT_EQ(MockStruct::move_creating_cnt, 0);
    EXPECT_EQ(MockStruct::copy_assignment_cnt, 0);
    EXPECT_EQ(MockStruct::move_assignment_cnt, 0);
    EXPECT_EQ(MockStruct::destructed_cnt, 0);
}

TEST(ImmutablePtrTest, PrimitiveTypeTest) {
    auto ptr = make_flat_immutable_ptr<int>(12345);
    EXPECT_EQ(*ptr, 12345);
}

TEST(ImmutablePtrTest, SimpleCopyConstructTest) {
    MockStruct::cleanCnts();
    auto ptr = make_flat_immutable_ptr<MockStruct>(42);
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
    auto ptr = make_flat_immutable_ptr<MockStruct>(42);
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
    ImmutablePtr<Derived> derivedPtr = make_flat_immutable_ptr<Derived>();

    // Auto Upcasting
    ImmutablePtr<Base> basePtr = derivedPtr; 
    ImmutablePtr<Derived> staticDerived = static_pointer_cast<Derived>(basePtr);
    if (auto dynamicDerived = dynamic_pointer_cast<Derived>(basePtr)) {
        dynamicDerived->foo();
    }
}

class TestPlayer {
public:
    std::string name;
    int hp;

    static inline size_t copy_cnt = 0;
    static inline size_t move_cnt = 0;

    TestPlayer(std::string n, int h) : name(std::move(n)), hp(h) {}
    
    // Отслеживаем деструкторы и конструкторы копирования/перемещения
    TestPlayer(const TestPlayer& other) : name(other.name), hp(other.hp) {
        ++copy_cnt;
    }
    TestPlayer(TestPlayer&& other) noexcept : name(std::move(other.name)), hp(other.hp) {
        ++move_cnt;
    }
};

TEST(ImmutablePtrTest, MutateTest) {
    TestPlayer::copy_cnt = 0;
    TestPlayer::move_cnt = 0;

    auto player1 = make_flat_immutable_ptr<TestPlayer>("Paladin", 100);
    
    EXPECT_EQ(TestPlayer::copy_cnt, 0);

    auto player2 = player1.mutate([](const TestPlayer& old_player) {
        TestPlayer updated{ old_player.name, old_player.hp - 30 }; 
        return updated;
    });

    EXPECT_EQ(player1->hp, 100);
    EXPECT_EQ(player2->hp, 70);
    EXPECT_EQ(player2->name, "Paladin");

    EXPECT_EQ(TestPlayer::copy_cnt, 0); 
}

// Helper structure that throws an exception during copy/move construction
struct ExplosivePlayer {
    int hp;
    bool should_explode = false;

    ExplosivePlayer(int h, bool explode) : hp(h), should_explode(explode) {}
    
    // Copy constructor that simulates an exception during mutation
    ExplosivePlayer(const ExplosivePlayer& other) : hp(other.hp), should_explode(other.should_explode) {
        if (should_explode) {
            throw std::runtime_error("Simulated crash inside object constructor during mutation!");
        }
    }
};

// =====================================================================
// MUTATE NULLPTR PROTECTION TEST
// =====================================================================
TEST(ImmutablePtrMutateSafetyTest, ThrowsOnNullptrMutation) {
    // Create an empty default-initialized ImmutablePtr
    ImmutablePtr<int> null_ptr;
    
    // Verify that calling mutate triggers a controlled std::runtime_error instead of SIGSEGV
    EXPECT_THROW({
        null_ptr.mutate([](const int& current) {
            return current + 10;
        });
    }, std::runtime_error);
}

// =====================================================================
// MUTATE EXCEPTION SAFETY TEST
// =====================================================================
TEST(ImmutablePtrMutateSafetyTest, ExceptionSafetyDuringPlacementNew) {
    // Create a valid pointer with an object configured to explode on copy/mutation
    auto player = make_flat_immutable_ptr<ExplosivePlayer>(100, true);
    ASSERT_TRUE(player);

    // Verify that if the mutator/constructor throws, the exception is safely propagated,
    // and the system RAII cleanup routines don't cause double-free or memory corruption
    EXPECT_THROW({
        auto corrupted_player = player.mutate([](const ExplosivePlayer& current) {
            ExplosivePlayer next{current}; // Triggers the explosive copy constructor
            next.hp -= 20;
            return next;
        });
    }, std::runtime_error);
    
    // Ensure the original player object remains untouched and perfectly valid
    EXPECT_EQ(player->hp, 100);
}
