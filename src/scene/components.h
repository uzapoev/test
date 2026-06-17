#ifndef __components_h__
#define __components_h__

#include "../mathlib.h"
#include "../common.h"
#include "../resource_manager.h"


struct icomponent 
{
    void* operator new(size_t) = delete;
    void* operator new[](size_t) = delete;
    void  operator delete(void*) = delete;
    void  operator delete[](void*) = delete;

   // class node * owner = nullptr;
   // friend class component_pool<icomponent>;

   // Explicitly allow placement new for our paged allocator.
    // This is the EXACT overload your component_pool uses under the hood!
    void* operator new(size_t, void* ptr) noexcept { return ptr; }
  //  void operator delete(void*, void*) noexcept {}
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
    uint64_t            parent_node_id;
    int16_t             child_count;

    uint64_t            next_sibling_node_id;
    uint64_t            prev_sibling_node_id;

    /*
    node *  parent() const { return parent; }
    void    add_child(node * n) {
    }*/
};

struct renderer : icomponent
{
    interned_string         mesh_guid;          // mesh guid
    interned_string         material_guid;      // material guid
    interned_string         lightmap_guid;      // lightmap guid
    vec4                    lightmap_scale_offset;
    uint32_t                flags;              // is_in_lodgroup, etc

    render_mesh_t*          mesh = nullptr;
    material_t*             material = nullptr;
    lightmap_t              lightmap;

    asset_handle_t          mesh_handle;

    uint32_t                material_count;
    guid_t*                 material_guids;

    mat4                    transform;      // global trransform
    aabbox                  local_bound;    // local aa bbox
    bbox                    world_bound;    // world aabbox
    vec4                    sphere_bound;   // world aphere bounds

   //SerializeObject(renderer,  SerializeObjectFieldWithKey("mesh", mesh_guid),
   //                           SerializeObjectFieldWithKey("mat_count", material_count),
   //                           SerializeObjectFieldWithKey("material",  materials),
   //                           SerializeObjectFieldWithKey("lightmap",  lightmap)
   // );
    //void set_material_param(const char * name, )
};


struct tiny_renderer
{
    asset_handle_t      mesh_handle;

    uint32_t            material_count;
    asset_handle_t*     material_handles;

    uint32_t            lightmap_idx;
    uint32_t            transform_idx;
};



struct occluder : icomponent
{
    interned_string         guid;
    class render_mesh_t *   mesh;
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


#endif