module;

#include <gc/gc.h>
#include <compare>
#include <type_traits>
#include <utility>
#include <new>
#include <cstdlib>
#include <memory>

export module immutable_ptr;

import scoped_gc_redirection;

namespace AmberRoom{

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

template <typename U, typename... Args>
friend ImmutablePtr<U> make_immutable_ptr(Args&&... args);

template<typename U> friend class ImmutablePtr;
template<typename To, typename From>
friend ImmutablePtr<To> static_pointer_cast(const ImmutablePtr<From>& r) noexcept;

template<typename To, typename From>
friend ImmutablePtr<To> dynamic_pointer_cast(const ImmutablePtr<From>& r) noexcept;


ImmutablePtr(const T* ptr): ptr_(ptr){}

const T* const ptr_;
};

template <typename T>
void gc_object_finalizer(void* /*obj*/, void* client_data) {
    if (client_data) {
        static_cast<T*>(client_data)->~T();
    }
}

export template <typename T, typename... Args>
ImmutablePtr<T> make_immutable_ptr(Args&&... args) {
    ScopedGCRedirection gcRedirection;
    
    void* mem = GC_malloc(sizeof(T));
    if (!mem) throw std::bad_alloc();
    
    // RAII-guard: if placement new failed then GC collect mem
    auto cleanup = [](void* p) { GC_free(p); };
    std::unique_ptr<void, decltype(cleanup)> guard(mem, cleanup);
    
    T* ptr = ::new (mem) T(std::forward<Args>(args)...);

    // register finalizer
    if constexpr (!std::is_trivially_destructible_v<T>) {
        void* hidden_ptr = reinterpret_cast<void*>(GC_HIDE_POINTER(ptr));

        // Если внутри T могут быть другие ImmutablePtr, замените на GC_register_finalizer
        GC_register_finalizer(
            mem, 
            [](void* /*obj*/, void* data) {
                T* real_ptr = static_cast<T*>(GC_REVEAL_POINTER(data));
                real_ptr->~T();
            }, 
            hidden_ptr, 
            nullptr, 
            nullptr
        );
    }

    guard.release();
    return ImmutablePtr<T>(ptr);
}

export template <typename T>
ImmutablePtr<T> clone_immutable_ptr(const ImmutablePtr<T>& ptr) {
    return make_immutable_ptr<T>(*ptr.get());
}

export template<typename To, typename From>
ImmutablePtr<To> static_pointer_cast(const ImmutablePtr<From>& r) noexcept {
    auto p = static_cast<const To*>(r.get());
    return ImmutablePtr<To>(p);
}

export template<typename To, typename From>
ImmutablePtr<To> dynamic_pointer_cast(const ImmutablePtr<From>& r) noexcept {
    if (auto p = dynamic_cast<const To*>(r.get())) {
        return ImmutablePtr<To>(p);
    }
    return ImmutablePtr<To>(nullptr);
}

} //namespace AmberRoom