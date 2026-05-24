#ifndef __resources_h__
#define __resources_h__

#include "gfx/gfx.h"
#include "mathlib.h"


typedef enum asset_type {
    asset_type_mesh,
    asset_type_image,
    asset_type_texture,
    asset_type_material,
    asset_type_shader,
    asset_type_font,
    asset_type_animation,
    asset_type_cinematic,
    asset_type_atlas,
    asset_type_prefab,
    asset_type_scene,
    asset_type_script,
    asset_type_userdata,
} asset_type;


struct material_instance_t {
    uint64_t                    uuid;
    char *                      keywords;   // keywords slpit by |, VERTEX|TEXTURE|PLANAR
    gfx_shader_t *              shader;
    gfx_render_states_desc_t    render_state;
};
/*
struct collision_mesh_t {
    uint64_t                    uuid;
    vertex                      vertexes; // 
    uint16_t *                  indexes;
};*/

struct animation_;

struct cinematic_;

typedef struct image_t {
    uint32_t width, height, depth, bpp;
    int8_t * data;
} image_t;


typedef struct shader_program_t {
    char *      keywords;
    int8_t      stage_count;

    struct {
        int16_t size;
        char*   data;
    } stage_data;
} shader_program_t;


typedef struct mesh_pool_t {
    gfx_offset_allocator_t*  vertex_buffer_allocator;
    gfx_offset_allocator_t*  index_buffer_allocator;

    int32_t                 vertex_buffer_size;
    int32_t                 index_buffer_size;

    int32_t                 vertex_buffer_offset;
    int32_t                 index_buffer_offset;

    gfx_buffer_t*           vertex_buffer;
    gfx_buffer_t*           index_buffer;
} mesh_pool_t;


typedef struct mesh_t {
    uint32_t                vertex_count;
    void*                   vertex_data;
    void*                   vertex_layout_count;

    struct {
        gfx_semantic        semantic;// pos
        gfx_vertex_format   format;
        uint32_t            stride;
    } vertex_layouts[16];

    gfx_index_format        index_format;
    uint32_t                index_count;
    void*                   index_data;

    uint32_t                submesh_count;
    uint32_t                submesh_indexes[16]; // start index

    aabbox                  bounds;
} mesh_t;

typedef struct submesh_t {
    uint32_t                start_index;
    uint32_t                index_count;
} submesh_t;

typedef struct render_mesh_t {
    //vertex layout
    uint32_t                vertex_stride;
    uint32_t                vertex_count;
    gfx_buffer_t*           vertex_buffer;
    uint32_t                vertex_buffer_offset;

    gfx_index_format        index_format;
    uint32_t                index_count;
    gfx_buffer_t*           index_buffer;
    uint32_t                index_buffer_offset;

    uint32_t                submesh_count;
    uint32_t                submeshes[16]; // start index

    aabbox                  bounds;
} render_mesh_t;




#endif // __resources_h__
