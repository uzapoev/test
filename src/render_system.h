#ifndef __render_system_h__
#define __render_system_h__

#include "gfx/gfx.h"
#include "common.h"
//#include "scene/scene.h"


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


typedef struct gfx_mesh_pool_t {

    int32_t                     vertex_buffer_size;
    int32_t                     index_buffer_size;

    int32_t                     vertex_buffer_offset;
    int32_t                     index_buffer_offset;

    gfx_buffer_t *              vertex_buffer;
    gfx_buffer_t *              index_buffer;
} gfx_mesh_pool_t;


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


typedef struct collision_mesh_t {
    uint32_t                vertex_count;
    uint32_t                vertex_stride;

    float *                 vertex_data;            // pos/normal/uv
    uint16_t *              index_data;             // pos/normal/uv
} collider_mesh_t;


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


namespace components {
    struct renderer;
    struct light;
    struct lodgroup;
};

struct attachements
{
    static constexpr char * color_attachement           = "_color_attachment";
    static constexpr char * depth_attachement           = "_depth_attachment";
    static constexpr char * motion_vectors_attachement  = "_motion_vectors_attachment";
    static constexpr char * hiz_attachement             = "_hiz_attachment";
    static constexpr char * cluster_attachement         = "_cluster_attachment";

    static constexpr char * gbuffer0_attachement        = "_gbuffer0_attachment";   // rgba8: rgb(albedo) a(ao)
    static constexpr char * gbuffer1_attachement        = "_gbuffer1_attachment";   // rgba8: normal
    static constexpr char * gbuffer2_attachement        = "_gbuffer2_attachment";   // rgba8: color-metal-roughness or diffuse-specular-glossiness

    static constexpr char * light_attachement           = "_light_attachment";      // rgba8: color-metal-roughness or diffuse-specular-glossiness
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
// class geometry_forward_pass  : render_pass { };
// class geometry_forward_plus_pass  : render_pass { };
// class geometry_deferred_pass  : render_pass { };
/*
    m_occlusion_pass->draw(cmd, scene);
    m_lightmap_pass->draw(cmd, scene);
* */


struct render_manager
{
public:
    components::renderer * allocate_renderer(/*scene, node**/) {
        //return m_allocator.allocate();
        return m_renderers[0]; 
    }

    void deallocate_renderer(components::renderer * _renderer) {
        //_renderer->material->release();
        //_renderer->mesh->release();
        //m_allocator.deallocate(_renderer);
    }

    components::lodgroup * allocate_lodgroup() {
        return m_lod_droups[0];
    };

    void deallocate_lodgroup(components::lodgroup * _renderer) {
        //_renderer->material->release();
        //_renderer->mesh->release();
        //m_allocator.deallocate(_renderer);
    }

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
    bool            is_valid(renderer_t* renderer)              { if(renderer != nullptr) return true; }

private:
    uint32_t                                m_free;
    std::vector < components::renderer*>    m_renderers;
    std::vector < components::light*>       m_lights;
    std::vector < components::lodgroup*>    m_lod_droups;

    std::vector<render_pass*>               m_passes;
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


static void draw_render_mesh(gfx_mesh_t * mesh, gfx_descriptor_set_t * set)
{
}



#endif // __resources_h__