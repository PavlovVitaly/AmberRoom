module;

#include <boost/pfr.hpp>
#include <type_traits>

export module restrictions;

namespace AmberRoom {

export template<typename T, typename... Args>
concept InitializableFrom = requires(Args&&... args) {
    ::new (std::declval<void*>()) T(std::forward<Args>(args)...);
} || (sizeof...(Args) > 0 && requires(Args&&... args) {
    ::new (std::declval<void*>()) T{std::forward<Args>(args)...};
});

// Trait helper to detect types that AmberRoom's objects has embeded pointer
template <typename T>
concept HasEmbeddedPointersMarker = requires {
    typename T::has_embedded_pointers;
};

// Strict compile-time check for an individual field type
template <typename T>
constexpr bool is_safe_field_v = 
    !std::is_pointer_v<T> && 
    !std::is_reference_v<T> && 
    !std::is_member_pointer_v<T> && 
    !HasEmbeddedPointersMarker<T> &&
    (std::is_scalar_v<T> || !std::is_compound_v<T>); // Compound types like std::string are blocked


export template <typename T>
concept PointerFree = []() constexpr {
    // Scalar primitives (int, double, bool) are always safe
    if constexpr (std::is_scalar_v<T>) {
        return true;
    }
    // Aggregate structures are dissected field-by-field at compile-time
    else if constexpr (std::is_aggregate_v<T>) {
        bool all_fields_safe = true;
        
        // boost::pfr::for_each_field extracts every type into a compile-time loop
        boost::pfr::for_each_field(T{}, [&](const auto& field) {
            using FieldType = std::decay_t<decltype(field)>;
            if constexpr (!is_safe_field_v<FieldType>) {
                all_fields_safe = false;
            }
        });
        
        return all_fields_safe;
    }
    return false;
}();

}