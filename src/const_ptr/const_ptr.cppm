module;

#include <gc/gc.h>
#include <compare>
#include <type_traits>
#include <utility>
#include <new>

export module const_ptr;

import scoped_gc_redirection;

namespace AmberRoom{

export template<typename T>
class ConstPtr{
public:


ConstPtr(const ConstPtr& other): ptr_(other.ptr_){}

ConstPtr(ConstPtr&& other) = delete;

// Auto upcasting (Derived* -> Base*)
template<typename U>
requires std::convertible_to<U*, T*>
ConstPtr(const ConstPtr<U>& other) : ptr_(other.get()) {}

ConstPtr& operator = (const ConstPtr& other){
    ptr_(other.ptr_);
};

template<typename U>
requires std::convertible_to<U*, T*>
ConstPtr& operator=(const ConstPtr<U>& other) noexcept {
    ptr_ = other.get();
    return *this;
}

ConstPtr& operator = (ConstPtr&& other) = delete;

~ConstPtr() = default;

const T* operator -> () const noexcept{
    return ptr_;
}

const T& operator * () const noexcept{
    return *ptr_;
};

bool operator == (const ConstPtr& other) const noexcept = default;

auto operator <=> (const ConstPtr& other) const noexcept = default;

template<typename U>
bool operator == (const ConstPtr<U>& other) const noexcept {
    return ptr_ == other.get();
}

template<typename U>
auto operator <=> (const ConstPtr<U>& other) const noexcept {
    return ptr_ <=> other.get();
}

explicit operator bool() const noexcept{
    return ptr_ != nullptr;
};

const T* get() const noexcept{
    return ptr_;
};

private:

template <typename U, typename... Args>
friend ConstPtr<U> make_const_ptr(Args&&... args);

template<typename U> friend class ConstPtr;
template<typename To, typename From>
friend ConstPtr<To> static_pointer_cast(const ConstPtr<From>& r) noexcept;

template<typename To, typename From>
friend ConstPtr<To> dynamic_pointer_cast(const ConstPtr<From>& r) noexcept;


ConstPtr(const T* ptr): ptr_(ptr){}

const T* const ptr_;
};

export template <typename T, typename... Args>
ConstPtr<T> make_const_ptr(Args&&... args) {
    ScopedGCRedirection gcRedirection;
    //void* mem = GC_MALLOC(sizeof(T));
    //if (!mem) throw std::bad_alloc();
    //T* ptr = ::new (mem) T(std::forward<Args>(args)...);
    T* ptr = new T(std::forward<Args>(args)...);

    // Register finalizator
    //if constexpr (!std::is_trivially_destructible_v<T>) {
    //    GC_register_finalizer_no_order(mem, [](void* obj, void* data) {
    //        static_cast<T*>(obj)->~T();
    //    }, nullptr, nullptr, nullptr);
    //}
    if constexpr (!std::is_trivially_destructible_v<T>) {
        // Boehm GC ожидает базовый адрес объекта
        GC_register_finalizer_no_order(static_cast<void*>(ptr), [](void* obj, void*) {
            static_cast<T*>(obj)->~T();
        }, nullptr, nullptr, nullptr);
    }

    return ConstPtr<T>(ptr);
}

export template <typename T>
ConstPtr<T> clone_const_ptr(const ConstPtr<T>& ptr) {
    return make_const_ptr<T>(*ptr.get());
}

export template<typename To, typename From>
ConstPtr<To> static_pointer_cast(const ConstPtr<From>& r) noexcept {
    auto p = static_cast<const To*>(r.get());
    return ConstPtr<To>(p);
}

export template<typename To, typename From>
ConstPtr<To> dynamic_pointer_cast(const ConstPtr<From>& r) noexcept {
    if (auto p = dynamic_cast<const To*>(r.get())) {
        return ConstPtr<To>(p);
    }
    return ConstPtr<To>(nullptr);
}

} //namespace AmberRoom