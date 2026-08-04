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

export struct FlatMode {};  
export struct GraphMode {}; 

export template<typename T, typename Mode = FlatMode>
class ImmutablePtr{
public:
using mode_type = Mode;

template<typename... Args>
requires InitializableFrom<T, Args...>
static ImmutablePtr<T, Mode> createInstance(Args&&... args) {
    ScopedGCRedirection gcRedirection;
    
    void* mem = nullptr;
    if(std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>){
        mem = GC_MALLOC_ATOMIC(sizeof(T));
    } else {
        mem = GC_MALLOC(sizeof(T));
    }
    
    if (!mem) throw std::bad_alloc();
    
    // RAII-guard: if placement new failed then GC collect mem
    auto cleanup = [](void* p) { GC_free(p); };
    std::unique_ptr<void, decltype(cleanup)> guard(mem, cleanup);
    
    T* ptr = ::new(mem) T(std::forward<Args>(args)...);

    // register finalizer
    if constexpr (!std::is_trivially_destructible_v<T>) {
        void* hidden_ptr = reinterpret_cast<void*>(GC_HIDE_POINTER(ptr));

        if constexpr (std::same_as<Mode, FlatMode>) {
            GC_REGISTER_FINALIZER_NO_ORDER(
                mem, 
                [](void* /*obj*/, void* data) {
                    T* real_ptr = static_cast<T*>(GC_REVEAL_POINTER(data));
                    real_ptr->~T();
                }, 
                hidden_ptr, 
                nullptr, 
                nullptr
            );
        } else {
            GC_REGISTER_FINALIZER(
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
    }

    guard.release();
    return ImmutablePtr<T, Mode>(ptr);
}

ImmutablePtr() : ptr_(nullptr) {};
ImmutablePtr(std::nullptr_t) : ptr_(nullptr) {};
ImmutablePtr(const ImmutablePtr& other): ptr_(other.ptr_){};
ImmutablePtr(ImmutablePtr&& other) = delete;

// Auto upcasting (Derived* -> Base*)
template<typename U>
requires std::convertible_to<U*, T*>
ImmutablePtr(const ImmutablePtr<U, Mode>& other) : ptr_(other.get()) {}

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
bool operator == (const ImmutablePtr<U, Mode>& other) const noexcept {
    return ptr_ == other.get();
}

template<typename U>
auto operator <=> (const ImmutablePtr<U, Mode>& other) const noexcept {
    return ptr_ <=> other.get();
}

explicit operator bool() const noexcept{
    return ptr_ != nullptr;
};

const T* get() const noexcept{
    return ptr_;
};

template<typename F>
requires Mutator<F, T>
auto mutate(F mutator) const{
    return createInstance(mutator(*ptr_));
}

private:

template<typename To, typename From, typename M>
requires requires(const From* f) { static_cast<const To*>(f); }
friend auto static_pointer_cast(const ImmutablePtr<From, M>& r) noexcept;

template<typename To, typename From, typename M>
requires std::is_polymorphic_v<From>
friend auto dynamic_pointer_cast(const ImmutablePtr<From, M>& r) noexcept;


ImmutablePtr(const T* ptr): ptr_(ptr){}

const T* const ptr_;
};

export template<typename U, typename Mode, typename... Args>
requires InitializableFrom<U, Args...>
auto make_immutable_ptr(Args&&... args) {
    return ImmutablePtr<U, Mode>::createInstance(std::forward<Args>(args)...);
}

export template<typename U, typename... Args>
requires InitializableFrom<U, Args...>
auto make_flat_immutable_ptr(Args&&... args) {
    return ImmutablePtr<U, FlatMode>::createInstance(std::forward<Args>(args)...);
}

export template<typename U, typename... Args>
requires InitializableFrom<U, Args...>
auto make_graph_immutable_ptr(Args&&... args) {
    return ImmutablePtr<U, GraphMode>::createInstance(std::forward<Args>(args)...);
}

export template<typename U, typename Mode>
requires std::copy_constructible<U>
auto clone_immutable_ptr(const ImmutablePtr<U, Mode>& ptr) {
    return make_immutable_ptr<U, Mode>(*ptr.get());
}

export template<typename To, typename From, typename M>
requires requires(const From* f) { static_cast<const To*>(f); }
auto static_pointer_cast(const ImmutablePtr<From, M>& r) noexcept {
    auto p = static_cast<const To*>(r.get());
    return ImmutablePtr<To, M>(p);
}

export template<typename To, typename From, typename Mode>
requires std::is_polymorphic_v<From>
auto dynamic_pointer_cast(const ImmutablePtr<From, Mode>& r) noexcept {
    if (auto p = dynamic_cast<const To*>(r.get())) {
        return ImmutablePtr<To, Mode>(p);
    }
    return ImmutablePtr<To, Mode>(nullptr);
}

} //namespace AmberRoom