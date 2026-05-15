#ifndef __gfx_dx12_h__
#define __gfx_dx12_h__

#include "gfx.h"

// DX12 backend is currently stubbed (compilation-only scaffolding).
// Enable by defining DX12_AVAILABLE (or having d3d12 headers present).
#if defined(_WIN32)
  #if !defined(DX12_AVAILABLE)
    #if __has_include(<d3d12.h>) && __has_include(<dxgi1_6.h>)
      #define DX12_AVAILABLE
    #endif
  #endif
#endif

#ifdef DX12_AVAILABLE

// Populates the function table with DX12 backend entrypoints.
// Implemented in `gfx_dx12.cpp`.
extern "C" void gfx_init_dx12(struct gfx_api_pfn* func_table);

#endif // DX12_AVAILABLE

#endif
