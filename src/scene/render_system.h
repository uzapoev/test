#ifndef __render_system_h__
#define __render_system_h__

#include "../gfx/gfx.h"
#include "../resources.h"
#include "../common.h"
#include "../memmgr.h"


template<class T>
struct shader_slot {
    interned_string     key;        // uniform name
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


typedef struct collision_mesh_t {
    uint32_t                vertex_count;
    uint32_t                vertex_stride;

    float *                 vertex_data;            // pos/normal/uv
    uint16_t *              index_data;             // pos/normal/uv
} collision_mesh_t;


typedef struct lightmap_t {
    gfx_texture_t*          lightmap;
    gfx_texture_t*          lightmask;
    vec4                    scale_offset;
} lightmap_t;


typedef struct renderer_t {
    render_mesh_t*          mesh = nullptr;
    gfx_material_t*         material = nullptr;
    lightmap_t              lightmap;

    mat4                    transform;
    aabbox                  bounds;
    bbox                    world_bounds;
    vec4                    sphere_bound;
} renderer_t;


typedef struct camera_data {
    mat4                    proj;
    mat4                    view;
    mat4                    view_proj;
    mat4                    inv_view;
    mat4                    inv_view_proj;
    vec4                    time;
    vec4                    view_port;
}camera_data;

typedef struct mesh_info {
};

typedef struct instance_data {
    mat4                    model;
    vec4                    sphere_bounds;
    uint32_t                pipeline_id;
    uint32_t                mesh_id;            // mesh id
    uint32_t                cell_id;            // spatial struct(cell id in grid struct)
} instance_data;


struct renderer;
struct light;
struct lodgroup;


struct attachements
{
    static constexpr char * color_attachement           = (char*)"_color_attachment";
    static constexpr char * depth_attachement           = (char*)"_depth_attachment";
    static constexpr char * motion_vectors_attachement  = (char*)"_motion_vectors_attachment";
    static constexpr char * hiz_attachement             = (char*)"_hiz_attachment";
    static constexpr char * cluster_attachement         = (char*)"_cluster_attachment";

    static constexpr char * gbuffer0_attachement        = (char*)"_gbuffer0_attachment";   // rgba8: rgb(albedo) a(ao)
    static constexpr char * gbuffer1_attachement        = (char*)"_gbuffer1_attachment";   // rgba8: normal
    static constexpr char * gbuffer2_attachement        = (char*)"_gbuffer2_attachment";   // rgba8: color-metal-roughness or diffuse-specular-glossiness

    static constexpr char * light_attachement           = (char*)"_light_attachment";      // rgba8:
};
/*
struct attachement_data {
    const char *            name;
    gfx_pixel_format        format;
    gfx_render_target_t*    target;
} attachement_config [] = {
    {attachements::color_attachement,           gfx_pixel_format_rgba8, nullptr },
    {attachements::depth_attachement,           gfx_pixel_format_d24s8, nullptr },
    {attachements::hiz_attachement,             gfx_pixel_format_d32,   nullptr },
    {attachements::motion_vectors_attachement,  gfx_pixel_format_rgba8, nullptr },

    {attachements::gbuffer0_attachement,        gfx_pixel_format_rgba8, nullptr },
    {attachements::gbuffer1_attachement,        gfx_pixel_format_rgba8, nullptr },
    {attachements::gbuffer2_attachement,        gfx_pixel_format_rgba8, nullptr },
    {attachements::light_attachement,           gfx_pixel_format_rgba8, nullptr },
};
*/
struct render_pass
{
public:
    virtual void    configure_input(/*attachements*/) = 0;
    virtual void    configure_output(/*attachements*/) = 0;

    virtual void    draw() = 0;
    bool            active() {return m_active;}
private:
    const char *    m_name;
    bool            m_active;
    uint32_t        m_queue_id;
};

class occlusion_pass : render_pass { };
class shadowmap_pass : render_pass { };
class geometry_pass  : render_pass { };
// class geometry_forward_pass : render_pass { };
// class geometry_forward_plus_pass : render_pass { };
// class geometry_deferred_pass : render_pass { };


struct render_system
{
public:
    static void create_and_make_shader(gfx_context_t* ctx) {
        s_shared = new render_system(ctx);
    }

    static render_system*  shared() { return s_shared; }

    render_system(gfx_context_t * ctx) ;

    renderer *  allocate_renderer();
    lodgroup *  allocate_lodgroup();
    light    *  allocate_light();


    void        deallocate_renderer(renderer * _renderer);
    void        deallocate_lodgroup(lodgroup * _lodgroup);

public:
    void enqueue_pass(render_pass * pass) {
        m_passes.push_back(pass);
    }

    void draw() {
        for(size_t i = 0; i < m_passes.size(); ++i) {
            if(!m_passes[i]->active())
                continue;
            m_passes[i]->draw(/*this*/);
        }
    }

private:
    static render_system*           s_shared;
    gfx_context_t*                  m_ctx = nullptr;

    std::vector<render_pass*>       m_passes;

    struct aligned_allocator *      m_allocator = nullptr;
    struct paged_pool_allocator*    m_renderer_allocator = nullptr;
    struct paged_pool_allocator*    m_light_allocator = nullptr;
    struct paged_pool_allocator*    m_occluder_allocator = nullptr;
    struct paged_pool_allocator*    m_lodgroup_allocator = nullptr;
};


static void draw_renderer(gfx_command_buffer_t * cmd, const renderer_t * renderer)
{
    auto mesh = renderer->mesh;
    auto set = renderer->material->descriptor_set;

    gfx_cmd_bind_descriptor_set(cmd, 0, set);
    gfx_cmd_bind_vertex_buffer(cmd, 0, mesh->vertex_buffer_offset, mesh->vertex_buffer);
    gfx_cmd_bind_index_buffer(cmd, mesh->index_format, mesh->index_buffer_offset, mesh->index_buffer);

    int32_t start_idx = 0;
    for (size_t sub_idx = 0; sub_idx < mesh->submesh_count; sub_idx++)
    {
        uint32_t count = mesh->submeshes[sub_idx];
        gfx_cmd_draw_indexed(cmd, count, start_idx, 1, 0);
        start_idx += mesh->submeshes[sub_idx];
    }

    if (mesh->submesh_count == 0)
    {
        gfx_cmd_draw_indexed(cmd, mesh->index_count, 0, 1, 0);
    }
}


static void draw_render_mesh(render_mesh_t* mesh, gfx_descriptor_set_t * set)
{
}



#endif // __resources_h__