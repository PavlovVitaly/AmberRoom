#include <gc.h>
#include <new>
#include <cstdlib>

import scoped_gc_redirection;

void* operator new(std::size_t size) {
    if (AmberRoom::g_use_gc_allocator) {
        void* ptr = GC_malloc(size);
        if (!ptr) throw std::bad_alloc();
        return ptr;
    }
    void* ptr = std::malloc(size);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

void operator delete(void* ptr) noexcept {
    if (ptr == nullptr) return;
    if (GC_is_heap_ptr(ptr)) {
        return; 
    }
    std::free(ptr);
}
