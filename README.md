# gfx(WIP)

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
    gfx_cmd_buffer_barrier(cmd, indirect_bufs, 1, gfx_barrier_compute_write, gfx_barrier_indirect);

    // 4. Graphics Phase: Execute Modern Mesh Shading Pass
    gfx_cmd_begin_pass(cmd, visibility_render_target);
    gfx_cmd_bind_pipeline(cmd, mesh_shading_pipeline);
    
    // Bindless descriptors & draw execution indirect loop
    gfx_cmd_bind_descriptor_set(cmd, 0, scene_data_set);
    gfx_cmd_draw_mesh_tasks_indirect(cmd, indirect_draw_buffer, 0, max_draw_commands, sizeof(gfx_indirect_data_t));

    gfx_cmd_end_pass(cmd);

    // 5. Submit queue and present frame
    gfx_end_frame(frame);
}
```

Example of use: Ray tracing
```c
// 1. Describe the geometry for BLAS
gfx_rt_geometry_desc_t geometry = {
    .vertex_buffer = mesh->vbo,
    .vertex_stride = sizeof(vertex_t),
    .vertex_count  = mesh->vertex_count,
    .vertex_format = GFX_VERTEX_FORMAT_FLOAT3,
    .index_buffer  = mesh->ibo,
    .index_count   = mesh->index_count,
    .index_format  = GFX_INDEX_FORMAT_UINT32,
    .flags         = gfx_rt_geometry_opaque
};

// 2. Create and assemble BLAS
gfx_acceleration_structure_desc_t blas_desc = {
    .label = "StaticMesh_BLAS",
    .is_top_level = false,
    .geometry_count = 1,
    .geometries = &geometry
};
gfx_acceleration_structure_t blas = gfx_acceleration_structure_create(ctx, &blas_desc);

// Write the build command (in reality, it is executed once when loading the mesh)
gfx_cmd_build_acceleration_structure(init_cmd_buf, blas, NULL);

// ... Time has passed, we are forming a frame ...

// 3. Fill the instance in instance_buffer (memory available to the GPU)
VkAccelerationStructureInstanceKHR instances[2] = {0};

    // First object (our Mesh)
    instances[0].transform = identity_matrix_3x4;
    instances[0].instanceCustomIndex = 0;
    instances[0].mask = 0xFF;
    instances[0].accelerationStructureReference = blas_mesh_1.device_address; // Используем GPU Address!

    // Second object (copy, offset to the side)
    instances[1].transform = translated_matrix_3x4;
    instances[1].instanceCustomIndex = 1;
    instances[1].mask = 0xFF;
    instances[1].accelerationStructureReference = blas_mesh_1.device_address;

gfx_buffer_update_data(ctx, tlas_instance_buffer, 0, sizeof(instances), instances);
// gfx_barrier_compute_read or special gfx_barrier_acceleration_structure
gfx_cmd_buffer_barrier(cmd, &tlas_instance_buffer, 1, gfx_barrier_transfer, gfx_barrier_compute_read); 

// 4. Create and assemble TLAS
gfx_acceleration_structure_desc_t tlas_desc = {
    .label = "Scene_TLAS",
    .is_top_level = true,
    .instance_count = 1,
    .instance_buffer = scene_instance_buffer
};
gfx_acceleration_structure_t tlas = gfx_acceleration_structure_create(ctx, &tlas_desc);
gfx_cmd_build_acceleration_structure(frame_cmd_buf, tlas, NULL);

// Don't forget the barrier: TLAS must be ready before tracing can begin
gfx_acceleration_structure_t tlas_arr[] = { tlas };
gfx_cmd_buffer_as_barrier(frame_cmd_buf, tlas_arr, 1, GFX_BARRIER_AS_WRITE, GFX_BARRIER_AS_READ);

// 5. Let's go! Let's trace rays
gfx_cmd_trace_rays(frame_cmd_buf, rt_pipeline, ray_shadows_sbt, screen_width, screen_height, 1);''
```


#emscripten build

cd projpath

mkdir build & cd build

emcmake cmake ..
cmake --build .
