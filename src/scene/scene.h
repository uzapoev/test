#ifndef __scene_h__
#define __scene_h__

#include <atomic>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>

#include "../mathlib.h"
#include "render_system.h"
#include "../resource_manager.h"
#include "components.h"
#include "../common.h"

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3) ((uint32_t)(ch0) | ((uint32_t)(ch1) << 8) | ((uint32_t)(ch2) << 16) | ((uint32_t)(ch3) << 24 ))
#endif

constexpr uint32_t const_hash(char const* input) {
    return *input ? static_cast<uint32_t>(*input) + 33 * const_hash(input + 1) : 5381;
}


// https://github.com/suVrik/acceleration_structure_benchmark
struct itree
{   
    struct node{};
    virtual void add(aabbox box, void * userdata)       = 0;
    virtual void rem(void* userdata)                    = 0;

    virtual void query(vec4 sphere, std::vector<node*> /*component_type */)  = 0;
    virtual void query(aabbox box,  std::vector<node*> /*component_type */)  = 0;
    virtual void query(frustum fr,  std::vector<node*> /*component_type */)  = 0;
};

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
    float m_far         = 500.0f;

   // vec4 _fanf          = { 45.0f , 4.0f / 3.0f ,0.1f , 50.0f };// fow, aspect, near, far
    quat _rot           = quat::identity();
    vec3 _pos           = math::Zero; 
    vec3 _target        = math::Backward;
    vec2 _axis;
    mat4 m_view_proj    = mat4::identity();
};


struct node
{
    interned_string         name;
    interned_string         guid;
    interned_string         tag;
    uint64_t                flags; // static, enabled

    uint32_t                index = 0;

    transform               transform;
    renderer                renderer;

  //  std::vector<components::icomponent*> m_components;
    std::vector<node>       childs;

private:
    hierarchy *             m_hierarchy = nullptr;
    //componentlist         m_components = nullptr;

    int                     parent = 0;
    int                     child_count = 0;
    int*                    child_indexes = nullptr;
};



struct tinynode
{
    interned_string         guid;
    interned_string         name;
    interned_string         tag;
    uint64_t                flags; // static, enabled

    uint64_t                id; 

    class transform *       m_transform = nullptr;
    class hierarchy *       m_hierarchy = nullptr;
    class componentlist *   m_components = nullptr;
};

class scene
{
public:
    enum component_type : uint32_t    {
        scene_meta_data         = const_hash("meta"),

        component_node,                                         // name, guid, tag, flags
        component_hierarchy     = const_hash("hierarhy"),       // parnet, childs;
        component_transform     = const_hash("transform"),      // 

        component_renderer      = const_hash("renderer"),       // mesh, materials, lightmap, renderparams(cast shadow, etc), occluder
        component_light         = const_hash("light"),          // point/dir/area, type(static/dynamic/mixed)
        component_lodgroup      = const_hash("lodgroup"),       // level of details
        component_occlusion     = const_hash("occlusion"),      // occlusion data
        component_streaming     = const_hash("streaming"),      // streaming data
        component_lightprobes   = const_hash("lightprobes"),    // streaming data

        component_collider      = const_hash("collider"),       // box, sphere, capsule, mesh, trigger
        component_rigidbody     = const_hash("rigidbody"),      // 

        component_animator      = const_hash("animator"),       // skinned mesh animator
        component_cinematic     = const_hash("cinematic"),      // kinda dotweens/ transform animations

        component_navagent      = const_hash("navagent"),       // pathfinding
        component_navmap        = const_hash("navmap"),         // pathfinding
        component_navobstacle   = const_hash("navobstacle"),    // pathfinding

        component_script        = const_hash("script"),         // scripts(backends: lua/c#/native)

        component_audio_ambient = const_hash("audio_ambient"),  // scripts(backends: lua/c#/native)
        component_audio_room    = const_hash("audio_room"),     // scripts(backends: lua/c#/native)
        component_audio_portal  = const_hash("audio_portal"),   // scripts(backends: lua/c#/native)

        component_userdata      = const_hash("userdata"),       // user data component
    };

    static scene                            create_from_json_file(const std::string_view& path);
    static scene                            create_from_xml_file(const std::string_view& path);
    static scene                            create_from_file(const std::string_view& path);

public:
    void                                    load(const std::string_view& path);
    void                                    save(const std::string_view& path);

    void                                    init();
    void                                    clear();
    void                                    update();
    void                                    draw(gfx_command_buffer_t* cmd, camera & cam);

    const std::vector<renderer_t> &         visible() const {return m_renderers;}

 public:
   node *       create_node(interned_string name = "", interned_string guid = "");
   transform *  create_transform();

public:
    void                                    traverse(node &root, std::function<void(node&)> &cb);

    const std::vector<renderer_t*> &        cull(const camera& camera);
public:
    friend struct scene_reader_json;
    friend struct scene_reader_xml;

    itree *                                 m_tree = nullptr;
    std::vector<renderer_t>                 m_renderers;
    std::vector<renderer_t*>                m_visibles;

    std::vector<node>                       m_nodes;
    std::vector<node*>                      m_nodes_flat_list;
    std::vector<node*>                      m_allocated_nodes;

    //scene resources
    std::unordered_set<interned_string>     m_meshes;
    std::unordered_set<interned_string>     m_materials;

    class world *                           m_world = nullptr;

    struct paged_pool_allocator *           m_node_allocator = nullptr;
    struct paged_pool_allocator *           m_transform_allocator = nullptr;
    struct paged_pool_allocator *           m_hierarchy_allocator = nullptr;
};

#endif