#ifndef __components_h__
#define __components_h__

#include "../mathlib.h"
#include "../common.h"



struct icomponent 
{
    class node * owner = nullptr;
};


struct transform : icomponent
{
    vec3                position;
    quat                rotation;
    vec3                scale;
        
    mat4                local_transform;
    mat4                global_transform;

    void update_transform() {
        local_transform = mat4::trs(position, rotation, scale);
        global_transform = local_transform;
    }
};

struct hierarchy : icomponent
{ 
    class node *        parent = nullptr;
    class node **       childs = nullptr;
    uint32_t            count = 0;
    /*
    node *  parent() const { return parent; }
    void    add_child(node * n) {
    }*/
};

struct renderer : icomponent
{
    interned_string     mesh_guid;          // mesh guid
    interned_string     material_guid;      // material guid
    interned_string     lightmap_guid;      // lightmap guid
    vec4                lightmap_scale_offset;
    uint32_t            flags;              // is_in_lodgroup, etc

    class render_mesh * mesh;
    class material *    material;
    friend class        render_system;
    friend class        resource_system;
};




struct occluder : icomponent
{
    interned_string     guid;
    class render_mesh * mesh;
};

struct lodgroup : icomponent
{
    float                   distance;
    std::vector<renderer>   renderers;
};

struct lightprobes : icomponent
{
    int probe_count;
    struct {
        float sh[9];
    } probes;
};
     
     
     
struct iphysic  : icomponent        { };
struct physic2d : iphysic           { };
struct physic3d : iphysic           { };
struct collider : icomponent        {
    enum { sphere, box, mesh };
};
struct box_collider : collider      { };
struct sphere_collider : collider   { };
struct capsule_collider : collider  { };
struct mesh_collider : collider     { };
struct rigidbody: iphysic           { };

struct navagent : icomponent    { };
struct animator : icomponent    { };
struct cinematic: icomponent    { };
struct script:    icomponent    { };



struct audio_object: icomponent
{
    void set_rtcp(int id, int value);
};

struct audio_listener: icomponent{};
struct audio_room: icomponent{};
struct audio_portal: icomponent{};
struct audio_ambient: icomponent{};
struct audio_bank;

class audio_system
{
    audio_listener * create_listener();
    audio_room *    create_room();
    audio_portal *  create_portal();
    audio_ambient * create_ambient();
    audio_bank *    load_bank();
};

#endif