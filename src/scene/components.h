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

    void* operator new(size_t, void* ptr) noexcept  { return ptr; }
    void  operator delete(void*, void*) noexcept    { }
};


struct hierarchy : icomponent
{
    guid_t              parent;

    uint64_t            parent_id;


    int16_t             child_count;
    uint64_t            next_sibling_id;
    uint64_t            prev_sibling_id;
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

    static void serialize(transform* _transform, binary_writer* bwr)
    {
        bwr->write(_transform->position);
        bwr->write(_transform->rotation);
        bwr->write(_transform->scale);
    }

    static void deserialize(transform* _transform, filestream* stream)
    {
        _transform->position = stream->read<vec3>();
        _transform->rotation = stream->read<quat>();
        _transform->scale    = stream->read<vec3>();
        _transform->update_transform();
    }
};



struct renderer : icomponent
{
    guid_t                  mesh_guid;              // mesh guid
    guid_t                  material_guid;          // material guid
    uint32_t                flags = 0;              // cast shadow, etc

    guid_t                  lightmap_color_guid;    // lightmap guid
    guid_t                  lightmap_mask_guid;     // lightmap guid
    vec4                    lightmap_scale_offset;  // scale offset

    render_mesh_t*          mesh = nullptr;
    material_t*             material = nullptr;

    asset_handle_t          mesh_handle;
    asset_handle_t          material_handle;

    uint32_t                material_count = 0;
    guid_t                  material_guids[8];

    mat4                    transform;      // global transform
  //  aabbox                  local_bound;    // local aa bbox
  //  bbox                    world_bound;    // world aabbox
    vec4                    sphere_bound;   // world aphere bounds

    inline bool             has_lightmap() noexcept { return lightmap_color_guid != 0;}


    static void serialize(icomponent* _component, binary_writer* bwr)
    {
        renderer* _renderer = static_cast<renderer*>(_component);
        bwr->write(_renderer->flags);
        bwr->write(_renderer->mesh_guid);
        bwr->write(_renderer->material_guid);

        bwr->write(_renderer->material_count);
        for(uint32_t i = 0; i < _renderer->material_count; ++i)
            bwr->write(_renderer->material_guids[i]);

        bwr->write(_renderer->lightmap_color_guid);
        bwr->write(_renderer->lightmap_mask_guid);
        bwr->write(_renderer->lightmap_scale_offset);
    }

    static void deserialize(icomponent* _component, filestream* stream)
    {
        renderer* _renderer                 = static_cast<renderer*>(_component);
        _renderer->flags                    = stream->read<uint32_t>();
        _renderer->mesh_guid                = stream->read<guid_t>();
        _renderer->material_guid            = stream->read<guid_t>();
        _renderer->material_count           = stream->read<uint32_t>();

    //    _renderer->material_guids = calloc(_renderer->material_count, sizeof(guid_t))
        for (uint32_t i = 0; i < _renderer->material_count; ++i)
            _renderer->material_guids[i] = stream->read<guid_t>();

        _renderer->lightmap_color_guid      = stream->read<guid_t>();
        _renderer->lightmap_mask_guid       = stream->read<guid_t>();
        _renderer->lightmap_scale_offset    = stream->read<vec4>();
    }

   //SerializeObject(renderer,  SerializeObjectFieldWithKey("mesh", mesh_guid),
   //                           SerializeObjectFieldWithKey("mat_count", material_count),
   //                           SerializeObjectFieldWithKey("material",  materials),
   //                           SerializeObjectFieldWithKey("lightmap",  lightmap)
   // );
    //void set_material_param(const char * name, )
};



struct lod_data
{
    float       screen_relative_transition_height;
    float       distance;
    guid_t      mesh_guid;      //List<string> meshGuids = new List<string>();
    guid_t      material_guid;  //List<List<string>> materialGuids = new List<List<string>>();
};

struct lodgroup : icomponent
{
    uint32_t            lod_count;
    guid_t              lod_mesh_guids[4];
    guid_t              lod_material_guids[4];

    float               lod_distance[4];
    float               lod_screen_relative_transition[4];


    static void serialize(icomponent* component, binary_writer* bwr) {
        lodgroup* c = static_cast<lodgroup*>(component);
        bwr->write(c->lod_count);
     // for(uint32_t i = 0; i < c->lod_count; ++i)
     //       bwr->write(c->lods[i]);
    }

    static void deserialize(icomponent* component, filestream* stream) {
        lodgroup* data = static_cast<lodgroup*>(component); 
    }
};


struct occluder : icomponent
{
    guid_t                      guid;   // occluder mesh guid
    struct render_mesh_t*       mesh;

    static void serialize(icomponent* component, binary_writer* bwr) {
        occluder* data = static_cast<occluder*>(component);
    }

    static void deserialize(icomponent* component, filestream* stream) {
        occluder* data = static_cast<occluder*>(component);
    }
};



struct lightprobes : icomponent
{
    int probe_count;
    struct {
        float sh[9];
    } probes;
};
     
struct streaming_component : icomponent
{
    float   load_distance;
    float   unload_distance;
    guid_t  scene_guid;
    ReflectObject(streaming_component, ReflectObjectField(scene_guid), ReflectObjectField(load_distance), ReflectObjectField(unload_distance))
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