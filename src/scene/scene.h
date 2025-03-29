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


enum component_type : uint16_t
{   
    component_unknown,      //

    component_node_begin,
    component_node_end,

    component_transform,    // 
    component_renderer,     // mesh, materials, lightmap, renderparams(cast shadow, etc)
    component_lodgroup,     // level of details
    component_streaming,    // streaming data
    component_collider,     // box, sphere, capsule, mesh
    component_rigidbody,    // 
    component_light,        // point/dir/area, 
    component_animator,     // skinned mesh animatoe
    component_cinematic,    // kinda dotweens/ transform animations
    component_navagent,     // pathfinding
    component_script,       // scripts(backends: lua/c#/native)
    component_custom,       // user data component
};

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
    const mat4& view_proj() const                               { return m_view_proj; }

    void move(const vec3& dir)                                  { _pos += dir; _target += dir; }
    void set_mouse_dt(const vec2& delta)                        { _axis += delta * 0.5; }

    void update();

    float m_fov         = 45.0f;
    float m_aspect      = 4.0f / 3.0f;
    float m_near        = 0.1f;
    float m_far         = 50.0f;
    vec4 _fanf          = { 45.0f , 4.0f / 3.0f ,0.1f , 50.0f };// Fow,Aspect,Near,Far
    quat _rot           = quat::identity();
    vec3 _pos           = math::Zero; 
    vec3 _target        = math::Backward;
    vec2 _axis;
    mat4 m_view_proj    = mat4::identity();
};


namespace components
{
    struct icomponent {  };

    struct transform : icomponent
    {
        vec3                    position;
        vec3                    scale;
        quat                    rotation;

        mat4                    local_transform;
        mat4                    global_transform;
    };

    struct renderer : icomponent
    {
        interned_string         mesh_guid;       // mesh guid
        interned_string         material_guid;   // material guid
        interned_string         lightmap_guid;   // lightmap guid
        vec4                    lightmap_scale_offset;
    };

    struct lodgroup : icomponent
    {
        float                   distance;
        std::vector<renderer>   renderers;
    };
     
    struct iphysic  : icomponent        { };
    struct physic2d : iphysic           { };
    struct physic3d : iphysic           { };
    struct collider : icomponent        { };
    struct box_collider : collider      { };
    struct sphere_collider : collider   { };
    struct capsule_collider : collider  { };
    struct mesh_collider : collider     { };

    struct rigidbody: iphysic       { };

    struct navagent : icomponent    { };
    struct animator : icomponent    { };
    struct cinematic: icomponent    { };
    struct script:    icomponent    { };
};


namespace handlers
{
    class itrigger
    {
        virtual void on_while_out() = 0;
        virtual void on_while_in() = 0;
        virtual void on_trigger_enter() = 0;
        virtual void on_trigger_exit() = 0;
    };

    class iphysic
    {
        virtual void collide(class iphysic * other) = 0;
    };
};


JsonSerializeExternal(vec2, 
    SerializeFieldWithKey("x", x), 
    SerializeFieldWithKey("y", y));

JsonSerializeExternal(vec3, 
    SerializeFieldWithKey("x", x), 
    SerializeFieldWithKey("y", y), 
    SerializeFieldWithKey("z", z));

JsonSerializeExternal(vec4, 
    SerializeFieldWithKey("x", x), 
    SerializeFieldWithKey("y", y), 
    SerializeFieldWithKey("z", z),
    SerializeFieldWithKey("w", w));

JsonSerializeExternal(quat, 
    SerializeFieldWithKey("x", x), 
    SerializeFieldWithKey("y", y), 
    SerializeFieldWithKey("z", z), 
    SerializeFieldWithKey("w", w));

JsonSerializeExternal(components::lodgroup,
    SerializeFieldWithKey("mesh", renderers));

JsonSerializeExternal(components::renderer,
    SerializeFieldWithKey("mesh", mesh_guid),
    SerializeFieldWithKey("material", material_guid),
    SerializeFieldWithKey("lightmap", lightmap_guid),
    SerializeFieldWithKey("lightmapScaleOffset", lightmap_scale_offset));

JsonSerializeExternal(components::transform,
    SerializeFieldWithKey("position", position),
    SerializeFieldWithKey("scale", scale),
    SerializeFieldWithKey("rotation", rotation)
);

struct node
{
    interned_string         name;
    interned_string         guid;
    interned_string         tag;
    uint64_t                flags; // static, enabled

    components::transform   transform;
    components::renderer    renderer;

  //  std::vector<components::icomponent*> m_components;
    std::vector<node>       childs;


    JsonSerialize(node,
        SerializeFieldWithKey("name", name),
        SerializeFieldWithKey("tag", tag),
        SerializeFieldWithKey("transform", transform),
        SerializeFieldWithKey("renderer", renderer),
        SerializeFieldWithKey("childs", childs)
    );
};


class scene
{
    friend class scene_reader_json;
    friend class scene_reader_xml;
public:
    static scene                            create_from_json_file(const std::string& path);
    static scene                            create_from_xml_file(const std::string& path);

public:
    void                                    load(const std::string& path);
    void                                    save(const std::string& path);

    void                                    init();
    void                                    update();
    void                                    draw(camera & cam);

    const std::vector<renderer_t> &         visible() const {return m_renderers;}

public:
    void                                    traverse(node &root, std::function<void(node&)> &cb);

    const std::vector<renderer_t*> &        cull(const mat4& mv);
    std::vector<pass>                       sort_by_passes(const std::vector<renderer_t> &);

public:
    itree *                                 m_tree;
    std::vector<renderer_t>                 m_renderers;
    std::vector<renderer_t*>                m_visibles;

    std::vector<node>                       m_nodes;
    std::vector<node*>                      m_nodes_flat_list;

    //scene resources
    std::unordered_set<interned_string>     m_meshes;
    std::unordered_set<interned_string>     m_materials;

private:
    JsonSerialize(scene,    SerializeFieldWithKey("childs",     m_nodes),
                            SerializeFieldWithKey("meshes",     m_meshes),
                            SerializeFieldWithKey("materials",  m_materials)
    );
};

#endif