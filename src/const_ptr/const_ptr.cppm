module;

#include <gc/gc.h>
#include <compare>
#include <type_traits>
#include <utility>
#include <new>

export module const_ptr;

namespace AmberRoom{

export template<typename T>
class ConstPtr{
public:


ConstPtr(const ConstPtr& other): val_(other.val_){}
ConstPtr(ConstPtr&& other) = delete;
ConstPtr& operator = (const ConstPtr& other){
    val_(other.val_);
}
ConstPtr& operator = (ConstPtr&& other) = delete;
~ConstPtr() = default;

const T* operator->() const {
    return val_;
}

const T* get() const{
    return val_;
}

private:

template <typename U, typename... Args>
friend ConstPtr<U> make_const_ptr(Args&&... args);

ConstPtr(T* val): val_(val){}

const T* const val_;
};

export template <typename T, typename... Args>
ConstPtr<T> make_const_ptr(Args&&... args) {
    void* mem = GC_MALLOC(sizeof(T));
    if (!mem) throw std::bad_alloc();
    T* ptr = ::new (mem) T(std::forward<Args>(args)...);
    return ConstPtr<T>(ptr);
}

export template <typename T>
ConstPtr<T> clone_const_ptr(const ConstPtr<T>& ptr) {
    return make_const_ptr<T>(*ptr.get());
}

} //namespace AmberRoom