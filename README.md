# gfx

A modern, light-weight, single-header C graphics abstraction layer designed for high-performance GPU-driven rendering, low memory footprint, and mobile-first efficiency. 

`gfx` provides a thin, deterministic interface over **Vulkan, Metal, and DirectX 12**, ditching heavy OOP abstractions in favor of **Orthodox C** and **Data-Oriented Design (DOD)**.

## Key Features

*   **Pure C99 API** — Zero OOP bloat, no complex handle hierarchies. Strictly data structures and functions.
*   **Modern Pipeline Focus** — Built-in support for Mesh Shading (`DrawMeshTasks`), Bindless Textures, and `ExecuteIndirect`.
*   **Hardware Ray Tracing** — Low-level BLAS/TLAS construction, binary-compatible instance descriptors, and managed Shader Binding Tables (SBT).
*   **Explicit Resource & Memory Control** — Explicit pipeline barriers, layout transitions, and deterministic `gfx_allocator_t` integration.
*   **Zero Fragmentation Allocation** — Built-in generation-salted generational handle pools (`gfx_handle_pool_t`) for fast, safe, cache-friendly lookups.

## Architecture Pipeline Overview

The library maps perfectly to modern GPU-driven paradigms (Proxy Rendering -> Hi-Z Culling -> Compute Filter -> ExecuteIndirect):




## Quick Start Code Example

Here is a quick glimpse of how a modern GPU-driven mesh shading pass looks using the `gfx` API:

```c
#define GFX_IMPLEMENTATION
#include "gfx.h"

void render_frame(gfx_context_t* ctx, gfx_surface_t* surface) {
    // 1. Begin frame and fetch ring-buffered command command tokens
    gfx_frame_t* frame = gfx_begin_frame(ctx, surface);
    gfx_command_buffer_t* cmd = frame->cmd_buffer;

    // 2. Compute Phase: Perform GPU-driven culling
    gfx_cmd_bind_pipeline(cmd, compute_cull_pipeline);
    gfx_cmd_bind_descriptor_set(cmd, 0, scene_data_set);
    gfx_cmd_dispatch_compute(cmd, (total_instances + 63) / 64, 1, 1);

    // 3. Insert memory barrier: Wait for Compute to write indirect args
    gfx_buffer_t* indirect_bufs[] = { indirect_draw_buffer };
    gfx_cmd_buffer_barrier(cmd, indirect_bufs, 1, GFX_BARRIER_COMPUTE_WRITE, GFX_BARRIER_INDIRECT_ARGUMENTS);

    // 4. Graphics Phase: Execute Modern Mesh Shading Pass
    gfx_cmd_begin_pass(cmd, visibility_render_target);
    gfx_cmd_bind_pipeline(cmd, mesh_shading_pipeline);
    
    // Bindless descriptors & draw execution indirect loop
    gfx_cmd_bind_descriptor_set(cmd, 0, scene_data_set);
    gfx_cmd_draw_mesh_tasks_indirect(cmd, indirect_draw_buffer, 0, max_draw_commands, sizeof(VkDrawMeshTasksIndirectCommandEXT));

    gfx_cmd_end_pass(cmd);

    // 5. Submit queue and present frame
    gfx_end_frame(frame);
}
```


#emscripten build

cd projpath

mkdir build & cd build

emcmake cmake ..
cmake --build .
