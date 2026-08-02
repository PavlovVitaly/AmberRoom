module;

export module scoped_gc_redirection;

namespace AmberRoom{

// redirection flag (new into GC or global new)
export inline thread_local bool g_use_gc_allocator = false;

export struct ScopedGCRedirection {
    ScopedGCRedirection()  { g_use_gc_allocator = true; }
    ~ScopedGCRedirection() { g_use_gc_allocator = false; }
};

} //namespace AmberRoom
