#ifndef __render_system_h__
#define __render_system_h__

#include "../gfx/gfx.h"
#include "../resources.h"
#include "../common.h"
#include "../memmgr.h"

#include "../json_serializer.h"
#include "ecs.h"


template<class T>
struct shader_slot {
    interned_string     key;        // uniform name
    T                   value = {};

    ReflectObject(shader_slot, ReflectObjectField(key), ReflectObjectField(value));
};

typedef struct material_instance_t {
    gfx_shader_t*                   shader = nullptr;
    gfx_pipeline_t*                 pipeline = nullptr;

    uint32_t                        uniform_count;
    gfx_uniform_t *                 uniforms;

    shader_slot<gfx_texture_t*>     textures[32];


    shader_slot<float4>             vectorsf[64];
    shader_slot<int4>               vectorsi[64];
    shader_slot<float>              scalarsf[64];
} material_instance_t;


typedef struct material_t {
    material_instance_t *           instance = nullptr;
    gfx_descriptor_set_t*           descriptor_set = nullptr;
} material_t;


typedef struct lightmap_t {
    gfx_texture_t*          lightmap    = nullptr;
    gfx_texture_t*          lightmask   = nullptr;
    vec4                    scale_offset;
} lightmap_t;


typedef struct camera_data {
    mat4                    proj;
    mat4                    view;
    mat4                    view_proj;
    mat4                    inv_view;
    mat4                    inv_view_proj;
    vec4                    time;
    vec4                    view_port;
}camera_data;

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


class render_system :public iquery_listener
{
public:
    static void create_and_make_shared(gfx_context_t* ctx) {
        s_shared = new render_system(ctx);
    }

    static render_system*  shared() { return s_shared; }

    render_system(gfx_context_t * ctx) ;


public:
    void enqueue_pass(render_pass * pass) {
        m_passes.push_back(pass);
    }

    void draw(gfx_command_buffer_t* cmd);
private:
    virtual void on_node_changed(struct tinynode* node, class component_manager& manager) override;

    virtual void garbage_collect() override;

private:
    static render_system*           s_shared;
    gfx_context_t*                  m_ctx = nullptr;

    std::vector<render_pass*>       m_passes;

  //  icomponent_query *              m_renderer_query;

    struct aligned_allocator *      m_allocator = nullptr;
    struct paged_pool_allocator*    m_renderer_allocator = nullptr;
    struct paged_pool_allocator*    m_light_allocator = nullptr;
    struct paged_pool_allocator*    m_occluder_allocator = nullptr;
    struct paged_pool_allocator*    m_lodgroup_allocator = nullptr;
};


#endif // __resources_h__