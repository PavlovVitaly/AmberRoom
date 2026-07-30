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

ConstPtr(T* val): val_(val){}

const T* operator->() {
    return val_;
} 

private:

const T* const val_;
};

export template <typename T, typename... Args>
ConstPtr<T> make_const_ptr(Args&&... args) {
    void* mem = GC_MALLOC(sizeof(T));
    if (!mem) throw std::bad_alloc();
    T* ptr = ::new (mem) T(std::forward<Args>(args)...);
    return ConstPtr<T>(ptr);
}

} //namespace AmberRoom