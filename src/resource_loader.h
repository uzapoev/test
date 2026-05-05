#ifndef __resource_loader_h__
#define __resource_loader_h__

#include "gfx/gfx.h"
#include "gfx/gfx_memory.h"
#include "mathlib.h"

#include "resources.h"

#include "mesh.h"

typedef enum texture_option {
    texture_option_none,
    texture_option_limit_dimension,
    texture_option_limit_size,
    texture_option_limit_mip
} texture_option;

typedef struct texture_load_option_t {
    texture_option  option  = texture_option_none;
    uint32_t        value   = 0; // 
} texture_load_option_t;


extern size_t   read_file_data(const char* path, char** data);

extern void     create_mesh_pool(gfx_context_t* ctx, uint32_t vertex_buffer_size, uint32_t index_buffer_size, mesh_pool_t* pool);

extern bool     load_mesh_from_file_path(gfx_context_t* ctx, mesh_pool_t* pool, const char* path, render_mesh_t* out_mesh);
extern void     load_mesh_from_file_data(gfx_context_t* ctx, mesh_pool_t* pool, const char* name, char* data, size_t size, render_mesh_t* out_mesh);

extern void     load_texture_from_file_path(gfx_context_t* ctx, const char* path, gfx_texture_t** out_texture);
extern void     load_texture_from_file_data(gfx_context_t* ctx, const char* name, char* data, size_t size, gfx_texture_t** out_texture);

extern void     load_shader_from_file_path(gfx_context_t* ctx, const char* path, gfx_shader_t** out_shader);
extern void     load_shader_from_file_data(gfx_context_t* ctx, const char* name, char* data, size_t size, gfx_shader_t** out_shader);

extern void     load_material_from_file_path(gfx_context_t* ctx, const char* path, struct gfx_material_instance_t** insance);
extern void     load_material_from_file_data(gfx_context_t* ctx, char* data, size_t size, struct gfx_material_instance_t** insance);

#endif 
