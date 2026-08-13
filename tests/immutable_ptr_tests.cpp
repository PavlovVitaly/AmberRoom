#include <gtest/gtest.h>
#include <gc/gc.h>
#include <stdexcept>
#include <cstdint>
#include <string>
#include <new>


import immutable_ptr;
import scoped_gc_redirection;

using AmberRoom::ImmutablePtr;
using AmberRoom::make_flat_immutable_ptr;
using AmberRoom::clone_immutable_ptr;
using AmberRoom::ScopedGCRedirection;

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

// Define a heavy over-aligned structure (simulating AVX-512 register state)
struct alignas(64) AlignedVectorState {
    double data[8];
};

// =====================================================================
// OVER-ALIGNMENT SAFETY TEST
// =====================================================================
TEST(ImmutablePtrAlignmentTest, VerifiedOverAlignedAllocation) {
    static bool gc_initialized = ([]() { GC_INIT(); return true; })();

    // Step 1: Allocate our 64-byte aligned structure via our safe factory
    auto vector = make_flat_immutable_ptr<AlignedVectorState>();
    ASSERT_TRUE(vector);

    // Step 2: Get the raw address returned by the garbage collector
    uintptr_t raw_address = reinterpret_cast<uintptr_t>(vector.get());

    // Step 3: Verify that the memory address is perfectly divisible by 64.
    // If our library used a basic GC_MALLOC, this check would fail on most systems,
    // exposing a fatal undefined behavior vulnerability.
    EXPECT_EQ(raw_address % 64, 0) << "Memory address " << raw_address 
                                   << " is not properly aligned to 64 bytes!";
}

// Helper over-aligned structure to trigger std::align_val_t overloads
struct alignas(64) HeavyGlobalState {
    uint64_t payload[8];
};

// =====================================================================
// GLOBAL NEW/DELETE REDIRECTION TESTS
// =====================================================================

TEST(GCRedirectionTest, VerifiedStdStringHeapInterception) {
    static bool gc_initialized = ([]() { GC_INIT(); return true; })();

    std::string* managed_string = nullptr;

    // Step 1: Trigger the redirection scope
    {
        ScopedGCRedirection redirection;
        
        // This invokes our overridden global operator new.
        // It must allocate a long string (> 15 chars) to force heap allocation and bypass SSO.
        managed_string = new std::string("This is a very long string that will definitely trigger heap allocation inside std::string");
    }

    ASSERT_NE(managed_string, nullptr);

    // Step 2: Verify that the string container itself belongs to the GC heap
    EXPECT_NE(GC_base(managed_string), nullptr) << "The string container bypassed ScopedGCRedirection!";

    // Step 3: Explicitly call global delete. 
    // Our updated deallocate_memory must use GC_base, detect it's a GC pointer,
    // invoke the destructor safely, and skip std::free to prevent heap corruption.
    delete managed_string;
}

TEST(GCRedirectionTest, VerifiedOverAlignedGlobalNewDelete) {
    static bool gc_initialized = ([]() { GC_INIT(); return true; })();

    HeavyGlobalState* aligned_obj = nullptr;

    // Step 1: Allocate over-aligned structure inside redirection scope
    {
        ScopedGCRedirection redirection;
        
        // This forces the compiler to choose operator new(size, std::align_val_t)
        aligned_obj = new HeavyGlobalState();
    }

    ASSERT_NE(aligned_obj, nullptr);

    // Step 2: Verify both the GC allocation and the 64-byte hardware alignment
    uintptr_t address = reinterpret_cast<uintptr_t>(aligned_obj);
    EXPECT_NE(GC_base(aligned_obj), nullptr) << "Aligned object bypassed GC memory!";
    EXPECT_EQ(address % 64, 0) << "Global aligned new failed to align address to 64 bytes!";

    // Step 3: Safe delete verification via GC_base
    delete aligned_obj;
}
