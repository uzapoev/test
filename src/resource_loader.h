#ifndef __resource_loader_h__
#define __resource_loader_h__

#include "gfx/gfx.h"
#include "mesh.h"

struct render_mesh_t;
struct material_json;
struct mesh_pool_t;
struct image_t;

extern uint32_t read_file_data(const char* path, bool binary, char** data);

extern void     create_mesh_pool(gfx_context_t* ctx, uint32_t vertex_buffer_size, uint32_t index_buffer_size, mesh_pool_t* pool);

extern bool     load_mesh_from_file_path(gfx_context_t* ctx, mesh_pool_t* pool, const char* path, render_mesh_t* out_mesh);
extern void     load_mesh_from_file_data(gfx_context_t* ctx, mesh_pool_t* pool, const char* name, char* data, uint32_t size, render_mesh_t* out_mesh);

extern void     load_image_from_file_path(const char* path, image_t ** out_image);
extern void     load_image_from_file_data(const char* name, char* data, uint32_t size, image_t ** out_image);

extern void     load_texture_from_file_path(gfx_context_t* ctx, const char* path, gfx_texture_t** out_texture);
extern void     load_texture_from_file_data(gfx_context_t* ctx, const char* name, char* data, uint32_t size, gfx_texture_t** out_texture);

extern void     load_shader_from_file_path(gfx_context_t* ctx, const char* path, gfx_shader_t** out_shader);
extern void     load_shader_from_file_data(gfx_context_t* ctx, const char* name, char* data, uint32_t size, gfx_shader_t** out_shader);

extern void     load_material_from_file_path(gfx_context_t* ctx, const char* path, struct material_t* material);
// `data` is a compiled material (resource_compile_material). Fills the uniform values (slot_names / slot_datas, one malloc block,
// free it with free_material_slots) and the texture names. The textures themselves are resolved by whoever owns the asset
// registry: pass `out_desc` to get the parsed description (texture guids, shader name). Returns false if the blob is invalid.
extern bool     load_material_from_file_data(gfx_context_t* ctx, char* data, uint32_t size, struct material_t* material, struct material_json* out_desc = nullptr);
extern void     free_material_slots(struct material_t* material);

#endif 
