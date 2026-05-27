#ifndef __scene_h__
#define __scene_h__

#include <atomic>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>

#include "../mathlib.h"
#include "../resource_manager.h"
#include "../common.h"

#include "render_system.h"
#include "components.h"

#include "ecs.h"

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






class entity_factory {
public:
    static entity_factory& instance() {
        static entity_factory inst;
        return inst;
    }

    entity_factory() {
        auto memory_resource = new aligned_allocator("entity_factory");
        m_node_allocator = new paged_pool_allocator(memory_resource, sizeof(tinynode), 1024 * 16);
    }

    tinynode* create_entity()
    {
        uint32_t assigned_id = 0;

        // Check if we can recycle an old ID to keep the pools dense
        if (!m_free_runtime_ids.empty()) {
            assigned_id = m_free_runtime_ids.front();
            m_free_runtime_ids.pop();
        }
        else {
            assigned_id = m_next_runtime_id++;
        }
        tinynode* tnode         = m_node_allocator->allocate<tinynode>();
        tnode->guid             = generate_random_guid();
        tnode->runtime_id       = assigned_id;
        tnode->component_mask   = 0;

        m_guid_to_node_map[tnode->guid] = tnode;

        return tnode;
    }

    tinynode* create_entity(guid_t persistent_guid) {
        uint32_t assigned_id = 0;
        if (!m_free_runtime_ids.empty()) {
            assigned_id = m_free_runtime_ids.front();
            m_free_runtime_ids.pop();
        }
        else {
            assigned_id = m_next_runtime_id++;
        }

        tinynode* tnode = create_entity();
        m_guid_to_node_map.erase(tnode->guid);
        tnode->guid = persistent_guid;
        m_guid_to_node_map[persistent_guid] = tnode;

        return tnode;
    }

    void destroy_entity(tinynode* node) {
        if (!node) return;
        m_free_runtime_ids.push(node->runtime_id);
        m_guid_to_node_map.erase(node->guid);
        node->~tinynode();
        m_node_allocator->deallocate(node);
    }

    // Quick runtime translation (useful for scripts working with asset references)
    tinynode* get_runtime_id(const guid_t& guid) const {
        auto it = m_guid_to_node_map.find(guid);
        return (it != m_guid_to_node_map.end()) ? it->second : nullptr;
    }

private:
    guid_t generate_random_guid() {
        // Your runtime GUID generation logic (e.g., MurmurHash from string or rand())
        return guid_t{ 0, 0 };
    }

private:
    paged_pool_allocator*                               m_node_allocator = nullptr;
    uint32_t                                            m_next_runtime_id = 0;
    std::queue<uint32_t>                                m_free_runtime_ids;
    std::unordered_map<guid_t, tinynode*, guid_hasher>  m_guid_to_node_map;
};


class scene
{
public:

    enum component_type : uint32_t    {
        scene_meta_data             = const_hash("meta"),

        component_node              /*= const_hash("node")*/,           // name, guid, tag, flags
        component_hierarchy         = const_hash("hierarhy"),       // parnet, childs;
        component_transform         = const_hash("transform"),      // 

        component_camera            = const_hash("camera"),
        component_renderer          = const_hash("renderer"),       // mesh, materials, lightmap, renderparams(cast shadow, etc), occluder
        component_decal             = const_hash("decal"),           // <-- NEW:
        component_reflection_probe  = const_hash("reflection_probe"), // <-- NEW:
        component_particle_system   = const_hash("particles"),    // <-- NEW: (VFX)

        component_light             = const_hash("light"),          // point/dir/area, type(static/dynamic/mixed)
        component_lodgroup          = const_hash("lodgroup"),       // level of details
        component_occlusion         = const_hash("occlusion"),      // occlusion data
        component_streaming         = const_hash("streaming"),      // streaming data
        component_lightprobes       = const_hash("lightprobes"),    // 
        component_volume_profile    = const_hash("volume_profile"),

        component_collider          = const_hash("collider"),       // box, sphere, capsule, mesh, trigger
        component_trigger           = const_hash("trigger"),
        component_rigidbody         = const_hash("rigidbody"),      // 

        component_animator          = const_hash("animator"),       // skinned mesh animator
        component_cinematic         = const_hash("cinematic"),      // kinda dotweens/ transform animations

        component_navagent          = const_hash("navagent"),       // pathfinding
        component_navmap            = const_hash("navmap"),         // pathfinding
        component_navobstacle       = const_hash("navobstacle"),    // pathfinding

        component_audio_ambient     = const_hash("audio_ambient"),  // 
        component_audio_room        = const_hash("audio_room"),     // 
        component_audio_portal      = const_hash("audio_portal"),   // 

        component_canvas            = const_hash("canvas"),
        component_script            = const_hash("script"),         // scripts(backends: lua/c#/native)
        component_userdata          = const_hash("userdata"),       // user data component
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


  //  entity_query<renderer_t>                m_render_query1;
    entity_query<transform, renderer>       m_render_query;
    std::vector<tinynode*>                  m_tiny_nodes;
    

    //scene resources
    std::unordered_set<interned_string>     m_meshes;
    std::unordered_set<interned_string>     m_materials;

    class world *                           m_world = nullptr;
    paged_pool_allocator *                  m_node_allocator = nullptr;
};

#endif