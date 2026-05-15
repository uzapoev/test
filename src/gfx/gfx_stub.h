#ifndef __gfx_stub_h__
#define __gfx_stub_h__

#include "gfx.h"

// Helper for backend stubs:
// - Logs warning if callback exists
// - Asserts in debug builds (so stubs don't silently ship into "working" code paths)
static inline void gfx_stub_not_implemented(gfx_callback dbglog, const char* what)
{
    if (dbglog && what)
        dbglog(gfx_msg_warning, "%s: not implemented", what);

#if !defined(NDEBUG)
    assert(!"gfx backend function not implemented");
#endif
}

#endif // __gfx_stub_h__

