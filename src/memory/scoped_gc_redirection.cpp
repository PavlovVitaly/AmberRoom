
#include <new>

import scoped_gc_redirection;

void* operator new(std::size_t size) {
    return AmberRoom::allocate_memory(size);
}

void* operator new[](std::size_t size) {
    return AmberRoom::allocate_memory(size);
}

void* operator new(std::size_t size, std::align_val_t al) {
    return AmberRoom::allocate_aligned_memory(size, static_cast<std::size_t>(al));
}

void* operator new[](std::size_t size, std::align_val_t al) {
    return AmberRoom::allocate_aligned_memory(size, static_cast<std::size_t>(al));
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

void operator delete(void* ptr, std::align_val_t) noexcept {
    operator delete(ptr);
}

void operator delete[](void* ptr, std::align_val_t) noexcept {
    operator delete[](ptr);
}

void operator delete(void* ptr, [[maybe_unused]] std::size_t size, std::align_val_t) noexcept {
    operator delete(ptr);
}

void operator delete[](void* ptr, [[maybe_unused]] std::size_t size, std::align_val_t) noexcept {
    operator delete[](ptr);
}
