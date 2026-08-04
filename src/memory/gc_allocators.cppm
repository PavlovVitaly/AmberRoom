module;

#include <gc/gc.h>
#include <memory>
#include  <cstdlib>

export module gc_allocators;

namespace AmberRoom {

export enum class GcScanKind {
    Normal,
    Atomic
};

// Универсальный аллокатор для связи STL и Boehm GC
export template <typename T, GcScanKind ScanKind>
class GcAllocator {
public:
    using value_type = T;

    GcAllocator() noexcept = default;
    template <typename U, GcScanKind SK> 
    GcAllocator(const GcAllocator<U, SK>&) noexcept {}

    [[nodiscard]] T* allocate(std::size_t n) {
        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
            throw std::bad_alloc();
        }
        
        void* ptr = nullptr;
        if constexpr (ScanKind == GcScanKind::Atomic) {
            ptr = GC_MALLOC_ATOMIC(n * sizeof(T));
        } else {
            ptr = GC_MALLOC(n * sizeof(T));
        }

        if (!ptr) throw std::bad_alloc();
        return static_cast<T*>(ptr);
    }

    void deallocate(T* ptr, std::size_t) noexcept {
        GC_free(ptr);
    }

    template <typename U, GcScanKind SK>
    bool operator==(const GcAllocator<U, SK>&) const noexcept { return true; }
};

} // namespace AmberRoom
