module;

#include <gc.h>
#include <new>
#include <cstdlib>

export module scoped_gc_redirection;

namespace AmberRoom{

// redirection flag (new into GC or global new)
export inline thread_local bool g_use_gc_allocator = false;

export struct ScopedGCRedirection {
    ScopedGCRedirection()  { g_use_gc_allocator = true; }
    ~ScopedGCRedirection() { g_use_gc_allocator = false; }
};

export void* allocate_memory(std::size_t size) {
    if (g_use_gc_allocator) {
        void* ptr = GC_MALLOC(size);
        if (!ptr) throw std::bad_alloc();
        return ptr;
    }
    void* ptr = std::malloc(size);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

export void* allocate_aligned_memory(std::size_t size, std::size_t alignment) {
    if (g_use_gc_allocator) {
        void* ptr = GC_memalign(alignment, size);
        if (!ptr) throw std::bad_alloc();
        return ptr;
    }
    // Standard aligned allocation fallback
#if defined(_those_win32_or_similar_) || defined(_MSC_VER)
    void* ptr = _aligned_malloc(size, alignment);
#else
    void* ptr = std::aligned_alloc(alignment, size);
#endif
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

export void deallocate_memory(void* ptr) noexcept {
    if (ptr == nullptr) return;
    if (GC_base(ptr) != nullptr) return;
    std::free(ptr);
}

} //namespace AmberRoom
