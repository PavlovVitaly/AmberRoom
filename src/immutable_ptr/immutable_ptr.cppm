module;

#include <gc/gc.h>
#include <compare>
#include <type_traits>
#include <utility>
#include <new>
#include <cstdlib>
#include <memory>
#include <concepts>

export module immutable_ptr;

import scoped_gc_redirection;

namespace AmberRoom{

template<typename T, typename... Args>
concept InitializableFrom = requires(Args&&... args) {
    ::new (std::declval<void*>()) T(std::forward<Args>(args)...);
} || (sizeof...(Args) > 0 && requires(Args&&... args) {
    ::new (std::declval<void*>()) T{std::forward<Args>(args)...};
});

template <typename F, typename U>
concept Mutator = std::invocable<F, const U&> && 
                     std::convertible_to<std::invoke_result_t<F, const U&>, U>;

export template<typename T>
class ImmutablePtr{
public:

ImmutablePtr() : ptr_(nullptr) {};
ImmutablePtr(std::nullptr_t) : ptr_(nullptr) {};
ImmutablePtr(const ImmutablePtr& other): ptr_(other.ptr_){};
ImmutablePtr(ImmutablePtr&& other) = delete;

// Auto upcasting (Derived* -> Base*)
template<typename U>
requires std::convertible_to<U*, T*>
ImmutablePtr(const ImmutablePtr<U>& other) : ptr_(other.get()) {}

ImmutablePtr& operator = (const ImmutablePtr& other) = delete;
ImmutablePtr& operator = (ImmutablePtr&& other) = delete;
~ImmutablePtr() = default;

const T* operator -> () const noexcept{
    return ptr_;
};

const T& operator * () const noexcept{
    return *ptr_;
};

bool operator == (const ImmutablePtr& other) const noexcept = default;
auto operator <=> (const ImmutablePtr& other) const noexcept = default;

template<typename U>
bool operator == (const ImmutablePtr<U>& other) const noexcept {
    return ptr_ == other.get();
}

template<typename U>
auto operator <=> (const ImmutablePtr<U>& other) const noexcept {
    return ptr_ <=> other.get();
}

explicit operator bool() const noexcept{
    return ptr_ != nullptr;
};

const T* get() const noexcept{
    return ptr_;
};

private:

template<typename U, typename... Args>
requires InitializableFrom<U, Args...>
friend ImmutablePtr<U> make_immutable_ptr(Args&&... args);

template<typename U, typename F>
requires Mutator<F, U>
friend ImmutablePtr<U> mutate(ImmutablePtr<U>& target, F mutator);

template<typename U> friend class ImmutablePtr;
template<typename To, typename From>
requires requires(const From* f) { static_cast<const To*>(f); }
friend ImmutablePtr<To> static_pointer_cast(const ImmutablePtr<From>& r) noexcept;

template<typename To, typename From>
requires std::is_polymorphic_v<From>
friend ImmutablePtr<To> dynamic_pointer_cast(const ImmutablePtr<From>& r) noexcept;


ImmutablePtr(const T* ptr): ptr_(ptr){}

const T* const ptr_;
};

export template<typename U, typename... Args>
requires InitializableFrom<U, Args...>
ImmutablePtr<U> make_immutable_ptr(Args&&... args) {
    ScopedGCRedirection gcRedirection;
    
    void* mem = GC_MALLOC(sizeof(U));
    if (!mem) throw std::bad_alloc();
    
    // RAII-guard: if placement new failed then GC collect mem
    auto cleanup = [](void* p) { GC_free(p); };
    std::unique_ptr<void, decltype(cleanup)> guard(mem, cleanup);
    
    U* ptr = ::new (mem) U(std::forward<Args>(args)...);

    // register finalizer
    if constexpr (!std::is_trivially_destructible_v<U>) {
        void* hidden_ptr = reinterpret_cast<void*>(GC_HIDE_POINTER(ptr));

        GC_register_finalizer(
            mem, 
            [](void* /*obj*/, void* data) {
                U* real_ptr = static_cast<U*>(GC_REVEAL_POINTER(data));
                real_ptr->~U();
            }, 
            hidden_ptr, 
            nullptr, 
            nullptr
        );
    }

    guard.release();
    return ImmutablePtr<U>(ptr);
}

export template<typename U>
requires std::copy_constructible<U>
ImmutablePtr<U> clone_immutable_ptr(const ImmutablePtr<U>& ptr) {
    return make_immutable_ptr<U>(*ptr.get());
}

export template<typename U, typename F>
requires Mutator<F, U>
ImmutablePtr<U> mutate(ImmutablePtr<U>& target, F mutator){
    return make_immutable_ptr<U>(mutator(*target));
}

export template<typename To, typename From>
requires requires(const From* f) { static_cast<const To*>(f); }
ImmutablePtr<To> static_pointer_cast(const ImmutablePtr<From>& r) noexcept {
    auto p = static_cast<const To*>(r.get());
    return ImmutablePtr<To>(p);
}

export template<typename To, typename From>
requires std::is_polymorphic_v<From>
ImmutablePtr<To> dynamic_pointer_cast(const ImmutablePtr<From>& r) noexcept {
    if (auto p = dynamic_cast<const To*>(r.get())) {
        return ImmutablePtr<To>(p);
    }
    return ImmutablePtr<To>(nullptr);
}

} //namespace AmberRoom