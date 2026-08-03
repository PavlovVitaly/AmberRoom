
#include <new>

import scoped_gc_redirection;

void* operator new(std::size_t size) {
    return AmberRoom::allocate_memory(size);
}

void* operator new[](std::size_t size) {
    return AmberRoom::allocate_memory(size);
}

void operator delete(void* ptr) noexcept {
    AmberRoom::deallocate_memory(ptr);
}

void operator delete[](void* ptr) noexcept {
    AmberRoom::deallocate_memory(ptr);
}

void operator delete(void* ptr, [[maybe_unused]] std::size_t size) noexcept {
    operator delete(ptr);
}

void operator delete[](void* ptr, [[maybe_unused]] std::size_t size) noexcept {
    operator delete[](ptr);
}
