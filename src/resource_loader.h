#ifndef __resource_loader_h__
#define __resource_loader_h__

#include "gfx/gfx.h"
#include "gfx/gfx_memory.h"
#include "mathlib.h"

typedef enum texture_option {
    texture_option_none,
    texture_option_limit_dimension,
    texture_option_limit_size,
    texture_option_limit_mip
} texture_option;

typedef struct texture_option_t {
    texture_option  option  = texture_option_none;
    uint32_t        value   = 0; // 
} texture_option_t;


struct gfx_material_instance_t;

struct gfx_mesh_pool_t;

struct gfx_mesh_t;

struct collision_mesh_t;

struct animation_;

struct cinematic_;


extern size_t   read_file_data(const char* path, char** data);

extern bool     load_mesh_from_file_path(gfx_context_t* ctx, gfx_mesh_pool_t* pool, const char* path, gfx_mesh_t* out_mesh);
extern void     load_mesh_from_file_data(gfx_context_t* ctx, gfx_mesh_pool_t* pool, const char* name, char* data, size_t size, gfx_mesh_t* out_mesh);

extern void     load_texture_from_file_path(gfx_context_t* ctx, const char* path, gfx_texture_t** out_texture);
extern void     load_texture_from_file_data(gfx_context_t* ctx, const char* name, char* data, size_t size, gfx_texture_t** out_texture);

extern void     load_shader_from_file_path(gfx_context_t* ctx, const char* path, gfx_shader_t** out_shader);
extern void     load_shader_from_file_data(gfx_context_t* ctx, const char* name, char* data, size_t size, gfx_shader_t** out_shader);

extern void     load_material_from_file_path(gfx_context_t* ctx, const char* path, struct gfx_material_instance_t** insance);
extern void     load_material_from_file_data(gfx_context_t* ctx, char* data, size_t size, struct gfx_material_instance_t** insance);

#endif // __resources_h__
