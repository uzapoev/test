#ifndef __render_manager_h__
#define __render_manager_h__

#include "gfx/gfx.h"
#include "common.h"

struct offset_allocator;


template<class T>
struct shader_slot {
    atomic_string   key;
    T               value = {};
};

typedef struct gfx_material_instance_t {
    gfx_shader_t*                   shader = nullptr;
    gfx_pipeline_t*                 pipeline = nullptr;

    shader_slot<gfx_texture_t*>     textures[16];
    shader_slot<vec4>               vectors[64];
    shader_slot<float>              scalars[64];
} gfx_material_instance_t;


typedef struct gfx_material_t {
    gfx_material_instance_t *       instance = nullptr;
    gfx_descriptor_set_t*           descriptor_set = nullptr;
}gfx_material_t;


typedef struct gfx_mesh_t {
    const char *        guid;
    gfx_index_format    format;
    uint32_t            vertex_count;
    gfx_buffer_t*       vertex_buffer;
    uint32_t            vertex_buffer_offset;

    uint32_t            index_count;
    gfx_buffer_t*       index_buffer;  
    uint32_t            index_buffer_offset;

    uint32_t            submesh_count;
    uint32_t            submeshes[16];

    aabbox              bounds;
}gfx_mesh_t;



typedef struct gfx_mesh_pool_t {
    offset_allocator*   vertex_allocator;
    offset_allocator*   index_allocator;

    uint32_t            vertex_buffer_size;
    uint32_t            index_buffer_size;

    gfx_buffer_t*       vertex_buffer;
    gfx_buffer_t*       index_buffer;
} gfx_mesh_pool_t;


typedef struct gfx_lightmap_t {
    gfx_texture_t*      lightmap;
    gfx_texture_t*      lightmask;
    vec4                scale_offset;
} gfx_lightmap_t;


typedef struct renderer_t
{
    gfx_mesh_t*             mesh = nullptr;
    gfx_material_t*         material = nullptr;
    gfx_lightmap_t          lightmap;

    mat4                    transform;
    aabbox                  bounds;
    aabbox                  world_bounds;
} renderer_t;



static void draw_renderer(gfx_command_buffer_t * cmd, const renderer_t * renderer)
{
    auto mesh = renderer->mesh;
    auto set = renderer->material->descriptor_set;

    gfx_cmd_bind_descriptor_set(cmd, set);
    gfx_cmd_bind_vertex_buffer(cmd, 0, mesh->vertex_buffer);
    gfx_cmd_bind_index_buffer(cmd, mesh->format, mesh->index_buffer);

    int32_t start_idx = 0;
    for (size_t sub_idx = 0; sub_idx < mesh->submesh_count; sub_idx++)
    {
        uint32_t count = mesh->submeshes[sub_idx];
        gfx_cmd_draw_indexed(cmd, count, start_idx, 1);
        start_idx += mesh->submeshes[sub_idx];
    }

    if (mesh->submesh_count == 0)
    {
        gfx_cmd_draw_indexed(cmd, mesh->index_count, 0, 1);
    }
}

#endif // __resources_h__