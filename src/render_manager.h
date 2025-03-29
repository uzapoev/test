#ifndef __render_manager_h__
#define __render_manager_h__

#include "gfx/gfx.h"
#include "common.h"

struct offset_allocator;


template<class T>
struct shader_slot {
    interned_string     key;        // 
    interned_string     guid;       // for textures
    T                   value = {};
};

typedef struct gfx_material_instance_t {
    gfx_shader_t*                   shader = nullptr;
    gfx_pipeline_t*                 pipeline = nullptr;

    shader_slot<gfx_texture_t*>     textures[32];
    shader_slot<float4>             vectorsf[64];
    shader_slot<int4>               vectorsi[64];
    shader_slot<float>              scalarsf[64];
} gfx_material_instance_t;


typedef struct gfx_material_t {
    gfx_material_instance_t *       instance = nullptr;
    gfx_descriptor_set_t*           descriptor_set = nullptr;
}gfx_material_t;


typedef struct gfx_mesh_t {
    uint32_t                vertex_count;
    gfx_buffer_t*           vertex_buffer;
    uint32_t                vertex_buffer_offset;

    gfx_index_format        index_format;
    uint32_t                index_count;
    gfx_buffer_t*           index_buffer;  
    uint32_t                index_buffer_offset;

    uint32_t                submesh_count;
    uint32_t                submeshes[16];

    aabbox                  bounds;
}gfx_mesh_t;


typedef struct collision_mesh_t
{
    uint32_t                vertex_count;
    uint32_t                vertex_stride;

    float *                 vertex_data;            // pos/normal/uv
    uint16_t *              index_data;             // pos/normal/uv
} collider_mesh_t;


typedef struct gfx_mesh_pool_t {
    offset_allocator*       vertex_allocator;
    offset_allocator*       index_allocator;

    uint32_t                vertex_buffer_size;
    uint32_t                index_buffer_size;

    uint32_t                vertex_buffer_offset;
    uint32_t                index_buffer_offset;

    gfx_buffer_t*           vertex_buffer;
    gfx_buffer_t*           index_buffer;
} gfx_mesh_pool_t;


typedef struct gfx_lightmap_t {
    gfx_texture_t*          lightmap;
    gfx_texture_t*          lightmask;
    vec4                    scale_offset;
} gfx_lightmap_t;


typedef struct renderer_t
{
    gfx_mesh_t*             mesh = nullptr;
    gfx_material_t*         material = nullptr;
    gfx_lightmap_t          lightmap;

    mat4                    transform;
    aabbox                  bounds;
    bbox                    world_bounds;
} renderer_t;


struct pass
{
    struct instance_batch
    {
        gfx_mesh_t *        mesh;
        uint32_t            count;
    };

    struct pass_batch
    {
        gfx_pipeline_t * pipeline;
        instance_batch * intsances;
    };
};


// current frame + previous frame
struct per_frame_data
{
    vec4    time;
    vec4    cam_pos;
    vec4    cam_dir;
    vec4    cam_param;  // near, far, fov,

    mat4    view;
    mat4    proj;
    mat4    view_proj;

    mat4    view_inv;
    mat4    proj_inv;
    mat4    view_proj_inv;
};

struct per_instance_data
{
    mat4    model;
    mat4    mvp;
};


static void draw_renderer(gfx_command_buffer_t * cmd, const renderer_t * renderer)
{
    auto mesh = renderer->mesh;
    auto set = renderer->material->descriptor_set;

    gfx_cmd_bind_descriptor_set(cmd, set);
    gfx_cmd_bind_vertex_buffer(cmd, 0, mesh->vertex_buffer_offset, mesh->vertex_buffer);
    gfx_cmd_bind_index_buffer(cmd, mesh->index_format, mesh->index_buffer_offset, mesh->index_buffer);

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