module;

#include <memory>
#include <string>
#include <vector>
#include <type_traits>

export module std_aliases;

import gc_allocators;

namespace AmberRoom {

export using gc_string = std::basic_string<
    char, 
    std::char_traits<char>, 
    GcAllocator<char, GcScanKind::Atomic>
>;

export template <typename T>
using gc_vector = std::vector<
    T, 
    GcAllocator<
        T, 
        std::is_trivially_copyable_v<T> ? GcScanKind::Atomic : GcScanKind::Normal
    >
>;

} // namespace AmberRoom
