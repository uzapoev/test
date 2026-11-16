#ifndef __components_h__
#define __components_h__

#include "../mathlib.h"
#include "../common.h"
#include "../resource_manager.h"
#include "../json_serializer.h"

struct icomponent 
{
    void* operator new(size_t) = delete;
    void* operator new[](size_t) = delete;
    void  operator delete(void*) = delete;
    void  operator delete[](void*) = delete;

    void* operator new(size_t, void* ptr) noexcept  { return ptr; }
    void  operator delete(void*, void*) noexcept    { }
};

struct light : icomponent 
{
    vec4    color;
    vec3    dir;
    float   radius;     // "radius": 10.0,
    float   intensity;  // "intensity" : 1.0,
    int     mode;       // "mode" : 0,
    int     shadowMode; // "shadowMode" : 2
};


// Links between the nodes of a scene by index in the node list (scene::m_nodes), built by scene::link_hierarchy() from
// node::parent (the guid of the parent, as it is in the file). The children of a node are a list in the order of the file:
// first_child -> next_sibling -> next_sibling...
struct hierarchy : icomponent
{
    static constexpr uint32_t none = 0xFFFFFFFFu;

    uint32_t            parent_index = none;    // none = a root
    uint32_t            first_child  = none;
    uint32_t            next_sibling = none;
    uint32_t            prev_sibling = none;
    int16_t             child_count  = 0;
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

    static void serialize(transform* _transform, binary_writer* bwr) {
        bwr->write(_transform->position);
        bwr->write(_transform->rotation);
        bwr->write(_transform->scale);
    }

    static void deserialize(transform* _transform, filestream* stream) {
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

    const render_mesh_t*        mesh = nullptr;
    const material_instance_t*  material = nullptr;

    asset_handle_t          mesh_handle;
    asset_handle_t          material_handle;

    guid_t                  material_guids[16];
    //small_array<guid_t, 12> material_guids;         // materials of the submeshes (City.json: 0..12, 95% have at most 1; an overflow is logged)

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

      //  bwr->write((uint32_t)_renderer->material_guids.size());
      //  for(const guid_t& material : _renderer->material_guids)
      //      bwr->write(material);

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
        const uint32_t material_count       = stream->read<uint32_t>();

     /*   _renderer->material_guids.clear();
        for (uint32_t i = 0; i < material_count; ++i) {
            const guid_t material = stream->read<guid_t>();         // always read, the stream stays in sync
            if (!_renderer->material_guids.push_back(material))
                debug::log_error("[renderer] more than %u materials, the rest is dropped", (unsigned)_renderer->material_guids.capacity());
        }*/

        _renderer->lightmap_color_guid      = stream->read<guid_t>();
        _renderer->lightmap_mask_guid       = stream->read<guid_t>();
        _renderer->lightmap_scale_offset    = stream->read<vec4>();
    }
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

    renderer            lod_renderers[4];


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
    enum mode {spherical_harmonics, sperical_gaussian, sperical_mixed };

    int     probe_count;

    vec3 *  harmonics; // one probe has 9 coefficients

    vec3*  axis;      // Direction of the petal center (normalized vector)
    float* sharpness; // Cone sharpness/width (how focused the light is)
    vec3*  amplitude; // Color and light intensity (RGB)
};
     
struct streaming_component : icomponent
{
    float   load_distance;
    float   unload_distance;
    guid_t  scene_guid;
};
     
struct collider : icomponent {  };

struct collider2d : icomponent{enum { circle, box, mesh };};
struct collider3d : icomponent{enum { sphere, box, mesh, capsule };};

struct rigidbody2d: icomponent { };
struct rigidbody3d: icomponent { };

struct navagent : icomponent    { };
struct animator : icomponent    { };
struct cinematic: icomponent    { };
struct script:    icomponent    { };


#endif