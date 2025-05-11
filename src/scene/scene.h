#ifndef __scene_h__
#define __scene_h__

#include <atomic>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>

#include "../mathlib.h"
#include "../render_system.h"
#include "../resource_manager.h"


enum component_type : uint16_t
{   
    component_unknown,      //

    component_node_begin,
    component_node_end,

    component_transform,    // 
    component_renderer,     // mesh, materials, lightmap, renderparams(cast shadow, etc), occluder
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


// https://github.com/suVrik/acceleration_structure_benchmark
struct itree
{   
    struct node{};
    virtual void add(aabbox box, void * userdata)       = 0;
    virtual void rem(void* userdata)                    = 0;

    virtual void query(aabbox box, std::vector<node*> /*component_type */)  = 0;
    virtual void query(frustum fr, std::vector<node*> /*component_type */)  = 0;
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
    float m_far         = 50.0f;

    vec4 _fanf          = { 45.0f , 4.0f / 3.0f ,0.1f , 50.0f };// fow, aspect, near, far
    quat _rot           = quat::identity();
    vec3 _pos           = math::Zero; 
    vec3 _target        = math::Backward;
    vec2 _axis;
    mat4 m_view_proj    = mat4::identity();
};


namespace components{struct icomponent;}

template<class T>
struct component_registrant
{
    component_registrant(T * t)
    {
//        assert(false);
        auto name = typeid(T).name();
        printf("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    }
/*
    component_registrant(char * name, components::icomponent*(create_fn*)())
    {
        
    }*/
};

struct component_registrant2
{
    component_registrant2()
    {
        assert(false);
        printf("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    }
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

        //REGISTER_COMPONENT(transform);
      //  static component_registrant<transform> registrant;
      //  static component_registrant2  registrant1;
     //   static constexpr component_registrant2  &registrant2 = registrant1;
    };



    struct renderer : icomponent //<component_transform>
    {
        interned_string         mesh_guid;          // mesh guid
        interned_string         material_guid;      // material guid
        interned_string         lightmap_guid;      // lightmap guid
        vec4                    lightmap_scale_offset;

        bool                    is_in_lodgroup;    //
        interned_string         occulder_guid;

    public:
        void clear()
        {
            mesh_guid.clear();
            material_guid.clear();
            lightmap_guid.clear();
            occulder_guid.clear();

       //     if(m_material)
       //         m_material->release();
        }

        gfx_mesh_t *            m_mesh;
        material   *            m_material;
    };

    struct light : icomponent { };
    struct occluder : icomponent        { };

    struct lodgroup : icomponent
    {
        float                       distance;
        std::vector<renderer*>      renderers;
    };
     
    struct iphysic          : icomponent { };
    struct collider         : icomponent { };
    struct box_collider     : collider { };
    struct sphere_collider  : collider { };
    struct capsule_collider : collider { };
    struct mesh_collider    : collider { };

    struct rigidbody        : icomponent { };

    struct navagent : icomponent    { };
    struct animator : icomponent    { };
    struct cinematic: icomponent    { };
    struct script:    icomponent    { };
    struct hierarchy: icomponent    { };
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



struct node
{
    interned_string         name;
    interned_string         guid;
    interned_string         tag;
    uint64_t                flags; // static, enabled

    uint64_t                id;

    components::transform   transform;
    components::renderer    renderer;

  //  std::vector<components::icomponent*> m_components;
    std::vector<node>       childs;

private:
    int                     parent = 0;
    int                     child_count = 0;
    int*                    child_indexes = nullptr;
};


struct tinynode
{
    interned_string         name;
    interned_string         guid;
    interned_string         tag;
    uint64_t                flags; // static, enabled

    uint64_t                id;

    //components::transform   transform;
    //components::renderer    renderer;

private:
    int                     parent = 0;
    int                     child_count = 0;
    int*                    child_indexes = nullptr;
};


class world
{
public:
    template<class T> T* allocate_component(class scene* , void* stream) 
    {
        return nullptr;
    };

  /*  template<> components::renderer* allocate_component(class scene*, void* stream) {
        //auto component = m_renderer->create_renderer();
        return m_renderer->allocate_renderer();
    }

    template<> components::collider* allocate_component(class scene*, void* stream) {
        //auto component = m_physic->create_collider();
        return nullptr;
    }*/

    render_manager*             m_renderer      = nullptr;
    class physic_manager *      m_physics2d     = nullptr;
    class physic_manager *      m_physics3d     = nullptr;
    class navigation_manager *  m_navigation    = nullptr;   // recast navmesh
//  uicanvas *                              m_canvas;       // ui renderer
};

class scene
{
    friend struct scene_reader_json;
    friend struct scene_reader_xml;
public:
    static scene                            create_from_json_file(const std::string& path);
    static scene                            create_from_xml_file(const std::string& path);
    static scene                            create_from_file(const std::string& path);

public:
    void                                    load(const std::string& path);
    void                                    save(const std::string& path);

    void                                    init();
    void                                    clear();
    void                                    update();
    void                                    draw(gfx_command_buffer_t* cmd, camera & cam);

    const std::vector<renderer_t> &         visible() const {return m_renderers;}

public:
    void                                    traverse(node &root, std::function<void(node&)> &cb);

    const std::vector<renderer_t*> &        cull(const mat4& mv);
public:
    itree *                                 m_tree;
    std::vector<renderer_t>                 m_renderers;
    std::vector<renderer_t*>                m_visibles;

    std::vector<node>                       m_nodes;
    std::vector<node*>                      m_nodes_flat_list;

    //scene resources
    std::unordered_set<interned_string>     m_meshes;
    std::unordered_set<interned_string>     m_materials;

    world                                   m_world;
//    render_manager *                        m_renderer;
};

#endif