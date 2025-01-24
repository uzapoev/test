#ifndef __scene_h__
#define __scene_h__

#include <atomic>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>

#include "../mathlib.h"
#include "../render_manager.h"
#include "../resource_manager.h"
#include "../json_serializer.h"



struct itree{};
struct kdtree   : itree {};
struct octree   : itree {};
struct quadtree : itree {};


struct camera
{
    void set_fov(float v)                                       { m_fov = v; }
    void set_apect(float v)                                     { m_aspect = v; }
    void set_near_far(float n, float f)                         { m_near = n; m_far = f; }

    void setup(float fov, float aspect, float near, float far)  { m_fov = fov;  m_aspect = aspect;  m_near = near;  m_far = far; }

    void set_pos(const vec3& v)                                 { _pos = v; }
    void set_target(const vec3& v)                              { _target = v; }

    vec3 forward()                                              { return math::mul(_rot, math::Forward); }
    vec3 left()                                                 { return math::mul(_rot, math::Left); }
    const mat4& vp() const                                      { return m_view_proj; }

    void move(const vec3& dir)                                  { _pos += dir; _target += dir; }
    void set_mouse_dt(const vec2& delta)                        { _axis += delta * 0.5; }

    void update();

    float m_fov         = 45.0f;
    float m_aspect      = 4.0f / 3.0f;
    float m_near        = 0.1f;
    float m_far         = 50.0f;
    quat _rot           = quat::identity();
    vec3 _pos           = math::Zero; 
    vec3 _target        = math::Backward;
    vec2 _axis;
    mat4 m_view_proj    = mat4::identity();
};


namespace components
{
    struct icomponent
    {
    };

    struct transform : icomponent
    {
        vec3                    position;
        vec3                    scale;
        quat                    rotation;
    };

    struct renderer : icomponent
    {
        atomic_string           mesh_guid;       // mesh guid
        atomic_string           material_guid;   // material guid
        atomic_string           lightmap_guid;   // lightmap guid
        vec4                    lightmap_scale_offset;
    };

    struct lodgroup
    {
        float                   distance;
        std::vector<renderer>   renderers;
    };
};

JsonSerializeExternal(components::renderer,
    SerializeFieldWithKey("mesh", mesh_guid),
    SerializeFieldWithKey("material", material_guid),
    SerializeFieldWithKey("lightmap", lightmap_guid),
    SerializeFieldWithKey("lightmapScaleOffset", lightmap_scale_offset)
);

JsonSerializeExternal(components::lodgroup,
    SerializeFieldWithKey("mesh", renderers)
);

JsonSerializeExternal(components::transform,
    SerializeFieldWithKey("position", position),
    SerializeFieldWithKey("scale", scale),
    SerializeFieldWithKey("rotation", rotation)
);

struct node
{
    atomic_string           name;
    atomic_string           tag;

    components::transform   transform;
    components::renderer    renderer;
    std::vector<node>       childs;

    JsonSerialize(node,
        SerializeFieldWithKey("name", name),
        SerializeFieldWithKey("tag", tag),
        SerializeFieldWithKey("transform", transform),
        SerializeFieldWithKey("renderer", renderer),
        SerializeFieldWithKey("childs", childs)
    );
};

struct pass 
{/*
    class instance_batch
    {
        gfx_mesh_t *        mesh;
        uint32_t            count;
    };

    class pass_batch
    {
        gfx_pipeline_t * pipeline;
        instance_batch * intsances;
    };

    */
    std::unordered_map<gfx_pipeline_t*, std::vector<int>> m;
};


class scene
{
public:
    static scene                    load(const std::string& path);

public:
    void                            update();
    void                            draw(camera & cam);

    const std::vector<renderer_t> & visible() const {return m_renderers;}

private:
    void                            traverse(node &, std::vector<node*> & allnodes);
    std::vector<renderer_t>         cull(const mat4& mv);
    std::vector<pass>               sort_by_passes(const std::vector<renderer_t> &);

public:
    itree *                         m_tree;
    std::vector<renderer_t>         m_renderers;
    std::vector<renderer_t>         m_visibles;
    std::vector<node>               m_nodes;

    std::vector<atomic_string>      m_textures;
    std::vector<atomic_string>      m_meshes;
    std::vector<atomic_string>      m_materials;
     
private:
    JsonSerialize(scene,    SerializeFieldWithKey("childs",     m_nodes),
                            SerializeFieldWithKey("textures",   m_textures),
                            SerializeFieldWithKey("meshes",     m_meshes),
                            SerializeFieldWithKey("materials",  m_materials)
    );
};



#endif