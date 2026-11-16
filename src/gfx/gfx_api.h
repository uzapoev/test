#ifndef __gfx_api_h__
#define __gfx_api_h__

// Internal backend function table shared by gfx.cpp and the backend implementations.
#include "gfx.h"

// Internal format helpers (defined in gfx.cpp).
// sRGB and linear pairs share the memory layout, only the sampling decode differs,
// so sizes / pitches / block info are computed on the linear format.
gfx_pixel_format    gfx_pixel_format_linear(gfx_pixel_format f);   // *_srgb -> linear pair, other formats unchanged
bool                gfx_pixel_format_is_srgb(gfx_pixel_format f);

 typedef struct gfx_api_pfn
 {
     // CONTEXT
     void     (*pfn_init)            (gfx_settings_t* settings, gfx_context_t** ctx);
     void     (*pfn_destroy)         (gfx_context_t* ctx);
     void     (*pfn_get_caps)        (gfx_context_t* ctx, gfx_caps_t* caps);

     // SWAPCHAIN / SURFACE
     void     (*pfn_surface_create)  (gfx_context_t* ctx, gfx_surface_desc_t* desc, gfx_surface_t** out_surface);
     void     (*pfn_surface_destroy) (gfx_context_t* ctx, gfx_surface_t* surface);
     gfx_result (*pfn_surface_set_vsync) (gfx_context_t* ctx, gfx_surface_t* surface, bool vsync);    // optional (null = not supported)

     void     (*pfn_frame_begin)     (gfx_context_t* ctx, gfx_surface_t* surface, gfx_frame_t** out_frame);
     void     (*pfn_frame_end)       (gfx_frame_t* frame);

     // BUFFER
     void     (*pfn_buffer_create)       (gfx_context_t* ctx, gfx_buffer_desc_t* desc, gfx_buffer_t** buffer);
     void     (*pfn_buffer_update_data)  (gfx_context_t* ctx, gfx_buffer_t* buffer, void* data, uint32_t size, uint32_t offset);
     void     (*pfn_buffer_destroy)      (gfx_context_t* ctx, gfx_buffer_t* buffer);

     // SHADER
     void    (*pfn_shader_create)                   (gfx_context_t* ctx, gfx_shader_desc_t* desc, gfx_shader_t** shader);
     uint32_t(*pfn_shader_get_descriptor_set_count) (gfx_shader_t* shader);
     uint32_t(*pfn_shader_get_uniforms)             (gfx_shader_t* shader, uint32_t set_index, gfx_uniform_t* uniforms); // new
     uint64_t(*pfn_uniform_location)                (gfx_shader_t* shader, const char* name);
     void    (*pfn_shader_destroy)                  (gfx_context_t* ctx, gfx_shader_t* shader);

     // DESCRIPTOR SET
     void     (*pfn_descriptor_set_create)               (gfx_context_t* ctx, gfx_shader_t* shader, uint32_t set_idx, gfx_descriptor_set_t** descriptor);
     void     (*pfn_descriptor_set_write_buffer_data)    (gfx_descriptor_set_t* set, uint64_t handle, void* data, uint32_t size);
     void     (*pfn_descriptor_set_write_buffer)         (gfx_descriptor_set_t* set, uint64_t handle, gfx_buffer_t* buffer, uint32_t offset);
     void     (*pfn_descriptor_set_write_texture)        (gfx_descriptor_set_t* set, uint64_t handle, gfx_texture_t* texture);
     void     (*pfn_descriptor_set_write_sampler)        (gfx_descriptor_set_t* set, uint64_t handle, gfx_sampler_t* sampler);
     void     (*pfn_descriptor_set_destroy)              (gfx_context_t* ctx, gfx_descriptor_set_t* descriptor);

     // SAMPLER
     void     (*pfn_sampler_create)  (gfx_context_t* ctx, gfx_sampler_desc_t* desc, gfx_sampler_t** sampler);
     void     (*pfn_sampler_destroy) (gfx_context_t* ctx, gfx_sampler_t* sampler);

     // TEXTURE
     void     (*pfn_texture_create)          (gfx_context_t* ctx, gfx_texture_desc_t* desc, gfx_texture_t** texture);
     void     (*pfn_texture_update_data)     (gfx_context_t* ctx, gfx_texture_t* texture, void* data, uint32_t size, uint32_t offset);
     void     (*pfn_texture_update_bindless) (gfx_context_t* ctx, gfx_texture_t* texture, uint32_t idx);
     void     (*pfn_texture_generate_mipmap) (gfx_context_t* ctx, gfx_texture_t* texture);
     void     (*pfn_texture_blit)            (gfx_context_t* ctx, gfx_texture_t* src, gfx_texture_t* dst);
     void     (*pfn_texture_get_data)        (gfx_context_t* ctx, gfx_command_buffer_t* cmd);
     void     (*pfn_texture_destroy)         (gfx_context_t* ctx, gfx_texture_t* texture);

     // PIPELINE
     void     (*pfn_pipeline_create)          (gfx_context_t* ctx, gfx_pipeline_desc_t* desc, gfx_pipeline_t** pipeline);
     void     (*pfn_mesh_pipeline_create)     (gfx_context_t* ctx, gfx_mesh_pipeline_desc_t* desc, gfx_pipeline_t** pipeline);
     void     (*pfn_pipeline_destroy)         (gfx_context_t* ctx, gfx_pipeline_t* pipeline);

     void     (*pfn_pipeline_compute_create)  (gfx_context_t* ctx, gfx_compute_pipeline_desc_t* desc, gfx_pipeline_compute_t** pipeline);
     void     (*pfn_pipeline_compute_destroy) (gfx_context_t* ctx, gfx_pipeline_compute_t* pipeline);

     void     (*pfn_pipeline_raytrace_create) (gfx_context_t* ctx, gfx_raytrace_pipeline_desc_t* desc, gfx_pipeline_raytrace_t** pipeline);
     void     (*pfn_pipeline_raytrace_destroy)(gfx_context_t* ctx, gfx_pipeline_raytrace_t* pipeline);

     // COMMAND BUFFER
     void     (*pfn_cmd_begin_pass)              (gfx_command_buffer_t* cmd, gfx_pass_info_t* pass_info);
     void     (*pfn_cmd_end_pass)                (gfx_command_buffer_t* cmd);
     void     (*pfn_cmd_begin_compute_pass)      (gfx_command_buffer_t* cmd);
     void     (*pfn_cmd_end_compute_pass)        (gfx_command_buffer_t* cmd);
     void     (*pfn_cmd_bind_compute_pipeline)   (gfx_command_buffer_t* cmd, gfx_pipeline_compute_t* pipeline);

     void     (*pfn_cmd_scissor) (gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
     void     (*pfn_cmd_viewport) (gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
     void     (*pfn_cmd_bind_pipeline) (gfx_command_buffer_t* cmd, gfx_pipeline_t* pipeline);
     void     (*pfn_cmd_bind_descriptor_set) (gfx_command_buffer_t* cmd, uint32_t slot, gfx_descriptor_set_t* descriptor);
     void     (*pfn_cmd_bind_index_buffer) (gfx_command_buffer_t* cmd, gfx_index_format format, uint32_t offset, gfx_buffer_t* buffer);
     void     (*pfn_cmd_bind_vertex_buffer) (gfx_command_buffer_t* cmd, uint32_t slot, uint32_t offset, gfx_buffer_t* buffer);
     void     (*pfn_cmd_draw) (gfx_command_buffer_t* cmd, uint32_t vertex_count, uint32_t instance_count);
     void     (*pfn_cmd_draw_indexed) (gfx_command_buffer_t* cmd, uint32_t idx_count, uint32_t first_idx, uint32_t instance_count, uint32_t vertex_offset);
     void     (*pfn_cmd_draw_indexed_indirect) (gfx_command_buffer_t* cmd, gfx_buffer_t* buffer, uint32_t offset, uint32_t draw_count, uint32_t stride);
     void     (*pfn_cmd_dispatch_compute) (gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t z);

     // Mesh Shading
     void     (*pfn_cmd_draw_mesh_tasks)             (gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t z);                    // new
     void     (*pfn_cmd_draw_mesh_tasks_indirect)    (gfx_command_buffer_t* cmd, gfx_buffer_t* buffer, uint32_t offset, uint32_t draw_count, uint32_t stride); // new

     // Ray Tracing
     void     (*pfn_acceleration_structure_create)   (gfx_context_t* ctx, gfx_acceleration_structure_desc_t* desc, gfx_acceleration_structure_t** acceleration_struct);
     void     (*pfn_acceleration_structure_destroy)  (gfx_context_t* ctx, gfx_acceleration_structure_t* acceleration_structure);
     void     (*pfn_sbt_create)                      (gfx_context_t* ctx, gfx_sbt_desc_t* desc, gfx_sbt_t** sbt);
     void     (*pfn_sbt_destroy)                     (gfx_context_t* ctx, gfx_sbt_t* sbt);
     void     (*pfn_cmd_build_acceleration_structure)(gfx_command_buffer_t* cmd, gfx_acceleration_structure_t* dst, gfx_acceleration_structure_t* src);
     void     (*pfn_cmd_trace_rays)                  (gfx_command_buffer_t* cmd, gfx_pipeline_raytrace_t* pipeline, gfx_sbt_t sbt, uint32_t width, uint32_t height, uint32_t depth);
     void     (*pfn_cmd_trace_ray_query)             (gfx_command_buffer_t* cmd, gfx_acceleration_structure_t tlas, uint32_t width, uint32_t height, uint32_t depth); // new

     // Barriers
     void     (*pfn_cmd_buffer_barrier)      (gfx_command_buffer_t* cmd, gfx_buffer_t** buffers, uint32_t count, gfx_barrier src, gfx_barrier dst);
     void     (*pfn_cmd_texture_barrier)     (gfx_command_buffer_t* cmd, gfx_texture_t** textures, uint32_t count, gfx_barrier src, gfx_barrier dst);

     // Debug
     void     (*pfn_cmd_push_marker)         (gfx_command_buffer_t* cmd, const char* marker);
     void     (*pfn_cmd_pop_marker)          (gfx_command_buffer_t* cmd);
     void     (*pfn_cmd_get_timestamps)      (gfx_command_buffer_t* cmd, gfx_timestamp_t* timestamps);

 } gfx_api_pfn;

#endif // __gfx_api_h__
